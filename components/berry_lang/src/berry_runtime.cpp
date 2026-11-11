#include "berry_runtime.h"

#include "berry.h"
#include "berry_internal.h"
#include "berry_source_buffer.h"
extern "C"
{
#include "be_gc.h"
}
#include "display.h"
#include "filesystem.h"
#include "network.h"
#include "text_console.h"
#include "ui.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

void berry_ui_begin_app(bvm *vm);
void berry_ui_end_app(void);
void berry_ui_set_key_state(uint8_t keycode, bool pressed);
void berry_ui_tick(void);

namespace
{
constexpr char TAG[] = "berry";
bvm *s_vm = nullptr;
constexpr char APP_GLOBAL[] = "_cos_berry_app";
constexpr int BERRY_GC_STEP_RATE_DEFAULT = 200;
int s_gc_step_rate = BERRY_GC_STEP_RATE_DEFAULT;
constexpr char CONSOLE_INPUT_CALLBACK_GLOBAL[] = "_cos_console_input_callback";
constexpr char MODULE_CACHE_GLOBAL[] = "_cos_modules";
std::recursive_mutex s_vm_mutex;
berry_console_read_line_fn s_console_read_line = nullptr;
berry_console_cancel_line_fn s_console_cancel_line = nullptr;
berry_console_read_key_fn s_console_read_key = nullptr;
berry_console_read_key_fn s_console_read_char = nullptr;
bool s_console_line_pending = false;
int s_console_line_token;
const char *s_app_class_global_name = nullptr;
constexpr int SCRIPT_PATH_STACK = 4;
constexpr size_t SCRIPT_TEXT_MAX = 4096;
char s_script_paths[SCRIPT_PATH_STACK][VFS_PATH_MAX];
int s_script_depth = 0;
char s_app_script[VFS_PATH_MAX];
bool s_app_script_set = false;

const char *current_script_path()
{
    if (s_script_depth > 0)
    {
        return s_script_paths[s_script_depth - 1];
    }
    if (s_app_script_set)
    {
        return s_app_script;
    }
    return nullptr;
}

void retain_app_script_path()
{
    const char *path = current_script_path();
    if (path == nullptr)
    {
        s_app_script_set = false;
        return;
    }

    const size_t length = std::strlen(path);
    if (length >= VFS_PATH_MAX)
    {
        s_app_script_set = false;
        return;
    }
    std::memcpy(s_app_script, path, length + 1);
    s_app_script_set = true;
}

bool can_emit_r5_checkpoint_log()
{
    // Best-effort guard: formatted stdio logging can allocate/flush and has triggered
    // heap-corruption panics in stdout_write_hook/fwrite under extreme fragmentation
    // (R5-FU1). Skip verbose checkpoint formatting when contiguous heap is critically low.
    return heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT) >= 1536;
}

void log_lifecycle_checkpoint(const char *phase)
{
    (void)phase;
    if (!can_emit_r5_checkpoint_log())
    {
        return;
    }
    const size_t berry_gc = s_vm != nullptr ? be_gc_memcount(s_vm) : 0;
    ESP_LOGI(TAG,
             "[R5] %s | berry_gc=%u heap_free=%u heap_min=%u heap_largest=%u internal_free=%u internal_min=%u "
             "internal_largest=%u",
             phase, static_cast<unsigned>(berry_gc), static_cast<unsigned>(esp_get_free_heap_size()),
             static_cast<unsigned>(esp_get_minimum_free_heap_size()),
             static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)),
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
             static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)),
             static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));
}

void log_heap_snapshot(const char *phase)
{
    log_lifecycle_checkpoint(phase);
}

void collect_berry_garbage()
{
    const size_t before = s_vm != nullptr ? be_gc_memcount(s_vm) : 0;
    if (s_vm != nullptr)
    {
        be_gc_collect(s_vm);
    }
    const size_t after = s_vm != nullptr ? be_gc_memcount(s_vm) : 0;
    const size_t reclaimed = before > after ? before - after : 0;
    if (!can_emit_r5_checkpoint_log())
    {
        return;
    }
    // Single R5 checkpoint log (reduces stdio pressure in low-heap/fragmented states).
    ESP_LOGI(TAG,
             "[R5] 7 GC complete | berry_gc=%u reclaimed=%u from=%u heap_free=%u heap_min=%u heap_largest=%u "
             "internal_free=%u internal_min=%u internal_largest=%u",
             static_cast<unsigned>(after), static_cast<unsigned>(reclaimed), static_cast<unsigned>(before),
             static_cast<unsigned>(esp_get_free_heap_size()), static_cast<unsigned>(esp_get_minimum_free_heap_size()),
             static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)),
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
             static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)),
             static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));
}

void clear_app_class_global(const char *class_name)
{
    if (class_name == nullptr)
    {
        return;
    }

    be_pushnil(s_vm);
    be_setglobal(s_vm, class_name);
    be_pop(s_vm, 1);
}

void close_app_from_ui()
{
    const esp_err_t err = berry_runtime_close_app();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not close Berry app from UI: %s", esp_err_to_name(err));
    }
}

int native_log(bvm *vm)
{
    if (be_top(vm) < 1 || !be_isstring(vm, 1))
    {
        be_return_nil(vm);
    }

    const char *message = be_tostring(vm, 1);
    ESP_LOGI(TAG, "%s", message != nullptr ? message : "");
    be_return_nil(vm);
}

int native_action(bvm *vm)
{
    if (be_top(vm) < 1 || !be_isstring(vm, 1))
    {
        be_return_nil(vm);
    }

    const char *command = be_tostring(vm, 1);
    int command_result = 0;
    const esp_err_t err = esp_console_run(command, &command_result);
    if (err != ESP_OK)
    {
        be_pushstring(vm, esp_err_to_name(err));
        be_return(vm);
    }

    be_pushint(vm, command_result);
    be_return(vm);
}

void clear_console_input_callback()
{
    be_pushnil(s_vm);
    be_setglobal(s_vm, CONSOLE_INPUT_CALLBACK_GLOBAL);
    be_pop(s_vm, 1);
}

esp_err_t cancel_pending_console_input()
{
    if (!s_console_line_pending)
    {
        return ESP_OK;
    }

    s_console_line_pending = false;
    clear_console_input_callback();
    if (s_console_cancel_line == nullptr)
    {
        return ESP_OK;
    }

    const esp_err_t err = s_console_cancel_line(&s_console_line_token);
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Could not cancel Berry console input: %s", esp_err_to_name(err));
    }
    return err == ESP_ERR_NOT_FOUND ? ESP_OK : err;
}

void console_line_complete(esp_err_t result, const char *line, void *context)
{
    (void)context;
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (s_vm == nullptr || !s_console_line_pending)
    {
        return;
    }

    s_console_line_pending = false;
    be_getglobal(s_vm, CONSOLE_INPUT_CALLBACK_GLOBAL);
    clear_console_input_callback();
    if (be_isfunction(s_vm, -1))
    {
        if (result == ESP_OK && line != nullptr)
        {
            be_pushstring(s_vm, line);
        }
        else
        {
            be_pushnil(s_vm);
        }
        if (be_pcall(s_vm, 1) != 0)
        {
            const char *error = be_tostring(s_vm, -1);
            ESP_LOGE(TAG, "Console input callback failed: %s", error != nullptr ? error : "unknown Berry error");
        }
        be_pop(s_vm, 2);
    }
    else
    {
        be_pop(s_vm, 1);
    }
}

int native_console_read_line(bvm *vm)
{
    if (be_top(vm) < 2 || !be_isstring(vm, 1) || !be_isfunction(vm, 2))
    {
        be_pushstring(vm, esp_err_to_name(ESP_ERR_INVALID_ARG));
        be_return(vm);
    }
    if (s_console_line_pending || s_console_read_line == nullptr)
    {
        be_pushstring(vm, esp_err_to_name(ESP_ERR_INVALID_STATE));
        be_return(vm);
    }

    be_pushvalue(vm, 2);
    be_setglobal(vm, CONSOLE_INPUT_CALLBACK_GLOBAL);
    be_pop(vm, 1);
    s_console_line_pending = true;
    const char *prompt = be_tostring(vm, 1);
    const esp_err_t err = s_console_read_line(prompt, console_line_complete, &s_console_line_token);
    if (err != ESP_OK)
    {
        s_console_line_pending = false;
        clear_console_input_callback();
        be_pushstring(vm, esp_err_to_name(err));
        be_return(vm);
    }

    be_pushbool(vm, 1);
    be_return(vm);
}

int native_console_cancel_line(bvm *vm)
{
    if (!s_console_line_pending || s_console_cancel_line == nullptr)
    {
        be_pushbool(vm, 0);
        be_return(vm);
    }

    const esp_err_t err = s_console_cancel_line(&s_console_line_token);
    if (err != ESP_OK)
    {
        be_pushstring(vm, esp_err_to_name(err));
        be_return(vm);
    }
    be_pushbool(vm, 1);
    be_return(vm);
}

void map_set_bool(bvm *vm, const char *key, bool value)
{
    be_pushstring(vm, key);
    be_pushbool(vm, value ? 1 : 0);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

void map_set_string(bvm *vm, const char *key, const char *value)
{
    be_pushstring(vm, key);
    be_pushstring(vm, value);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

void map_set_number(bvm *vm, const char *key, double value)
{
    be_pushstring(vm, key);
    be_pushreal(vm, static_cast<breal>(value));
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

void map_set_int(bvm *vm, const char *key, bint value)
{
    be_pushstring(vm, key);
    be_pushint(vm, value);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

int push_fs_error(bvm *vm, esp_err_t err)
{
    be_newobject(vm, "map");
    map_set_bool(vm, "ok", false);
    map_set_string(vm, "error", esp_err_to_name(err));
    be_pop(vm, 1);
    be_return(vm);
}

bool path_has_prefix(const char *path)
{
    if (path == nullptr || path[0] == '\0')
    {
        return false;
    }
    if (path[0] == '/')
    {
        return true;
    }
    return path[0] == '.' && path[1] == '/';
}

bool read_script_path(bvm *vm, char *resolved, esp_err_t *error)
{
    if (be_top(vm) < 1 || !be_isstring(vm, 1))
    {
        *error = ESP_ERR_INVALID_ARG;
        return false;
    }

    const char *path = be_tostring(vm, 1);
    if (!path_has_prefix(path))
    {
        be_raise(vm, "value_error", "path must start with '/' or './'");
    }

    const esp_err_t composed = vfs_compose_script_path(current_script_path(), path, resolved, VFS_PATH_MAX);
    if (composed == ESP_ERR_INVALID_STATE)
    {
        be_raise(vm, "value_error", "relative paths need a script file");
    }
    if (composed == ESP_ERR_INVALID_ARG)
    {
        be_raise(vm, "value_error", "path escapes the filesystem root");
    }
    if (composed == ESP_ERR_INVALID_SIZE)
    {
        be_raise(vm, "value_error", "path is too long");
    }
    if (composed != ESP_OK)
    {
        be_raise(vm, "value_error", "path must start with '/' or './'");
    }
    if (!vfs_is_virtual_path(resolved))
    {
        *error = ESP_ERR_INVALID_ARG;
        return false;
    }

    *error = ESP_OK;
    return true;
}

bool berry_json_value(bvm *vm, int index)
{
    return be_isnil(vm, index) || be_isbool(vm, index) || be_isnumber(vm, index) || be_isstring(vm, index) ||
           be_islist(vm, index) || be_islistinstance(vm, index) || be_ismap(vm, index) || be_ismapinstance(vm, index);
}

extern "C" bool berry_runtime_ensure_json(bvm *vm)
{
    if (be_getglobal(vm, "json") && !be_isnil(vm, -1))
    {
        be_pop(vm, 1);
        return true;
    }
    be_pop(vm, 1);

    if (!be_getmodule(vm, "json"))
    {
        return false;
    }
    be_setglobal(vm, "json");
    be_pop(vm, 1);
    return true;
}

bool call_json_method(bvm *vm, const char *method, int value_index)
{
    value_index = be_absindex(vm, value_index);
    const int base = be_top(vm);
    if (!berry_runtime_ensure_json(vm) || !be_getglobal(vm, "json") || !be_getmember(vm, -1, method) ||
        be_isnil(vm, -1))
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }

    be_remove(vm, -2);
    be_pushvalue(vm, value_index);
    const int status = be_pcall(vm, 1);
    if (status != 0)
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }
    if (be_top(vm) == base + 2)
    {
        be_remove(vm, base + 2);
    }
    if (be_top(vm) != base + 1)
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }
    return true;
}

bool json_whitespace(char character)
{
    return character == ' ' || character == '\t' || character == '\r' || character == '\n';
}

int push_write_status(bvm *vm, esp_err_t err)
{
    if (err == ESP_OK)
    {
        be_pushbool(vm, 1);
    }
    else
    {
        be_pushstring(vm, esp_err_to_name(err));
    }
    be_return(vm);
}

esp_err_t begin_console_callback(bvm *vm, int function_index, berry_console_read_key_fn start)
{
    if (start == nullptr || s_console_line_pending)
    {
        return ESP_ERR_INVALID_STATE;
    }

    be_pushvalue(vm, function_index);
    be_setglobal(vm, CONSOLE_INPUT_CALLBACK_GLOBAL);
    be_pop(vm, 1);
    s_console_line_pending = true;
    const esp_err_t err = start(console_line_complete, &s_console_line_token);
    if (err != ESP_OK)
    {
        s_console_line_pending = false;
        clear_console_input_callback();
    }
    return err;
}

int native_console_read_key(bvm *vm)
{
    if (be_top(vm) < 1 || !be_isfunction(vm, 1))
    {
        be_pushstring(vm, esp_err_to_name(ESP_ERR_INVALID_ARG));
        be_return(vm);
    }

    const esp_err_t err = begin_console_callback(vm, 1, s_console_read_key);
    if (err == ESP_OK)
    {
        be_pushbool(vm, 1);
    }
    else
    {
        be_pushstring(vm, esp_err_to_name(err));
    }
    be_return(vm);
}

int native_console_read_char(bvm *vm)
{
    if (be_top(vm) < 1 || !be_isfunction(vm, 1))
    {
        be_pushstring(vm, esp_err_to_name(ESP_ERR_INVALID_ARG));
        be_return(vm);
    }

    const esp_err_t err = begin_console_callback(vm, 1, s_console_read_char);
    if (err == ESP_OK)
    {
        be_pushbool(vm, 1);
    }
    else
    {
        be_pushstring(vm, esp_err_to_name(err));
    }
    be_return(vm);
}

esp_err_t parse_console_color(const char *name, text_console_color_t *color)
{
    if (name == nullptr || color == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (std::strcmp(name, "default") == 0)
    {
        *color = TEXT_CONSOLE_COLOR_DEFAULT;
    }
    else if (std::strcmp(name, "black") == 0)
    {
        *color = TEXT_CONSOLE_COLOR_BLACK;
    }
    else if (std::strcmp(name, "red") == 0)
    {
        *color = TEXT_CONSOLE_COLOR_RED;
    }
    else if (std::strcmp(name, "green") == 0)
    {
        *color = TEXT_CONSOLE_COLOR_GREEN;
    }
    else if (std::strcmp(name, "yellow") == 0)
    {
        *color = TEXT_CONSOLE_COLOR_YELLOW;
    }
    else if (std::strcmp(name, "blue") == 0)
    {
        *color = TEXT_CONSOLE_COLOR_BLUE;
    }
    else if (std::strcmp(name, "magenta") == 0)
    {
        *color = TEXT_CONSOLE_COLOR_MAGENTA;
    }
    else if (std::strcmp(name, "cyan") == 0)
    {
        *color = TEXT_CONSOLE_COLOR_CYAN;
    }
    else if (std::strcmp(name, "white") == 0)
    {
        *color = TEXT_CONSOLE_COLOR_WHITE;
    }
    else
    {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t optional_console_color(bvm *vm, int index, int argc, text_console_color_t *color)
{
    if (argc < index)
    {
        *color = TEXT_CONSOLE_COLOR_CURRENT;
        return ESP_OK;
    }
    if (!be_isstring(vm, index))
    {
        return ESP_ERR_INVALID_ARG;
    }
    return parse_console_color(be_tostring(vm, index), color);
}

bool berry_int_arg(bvm *vm, int index, int *value)
{
    if (!be_isint(vm, index))
    {
        return false;
    }
    const bint raw = be_toint(vm, index);
    *value = static_cast<int>(raw);
    return true;
}

void push_console_status(bvm *vm, esp_err_t err)
{
    if (err == ESP_OK)
    {
        be_pushbool(vm, 1);
    }
    else
    {
        be_pushstring(vm, esp_err_to_name(err));
    }
}

int native_console_move(bvm *vm)
{
    int x = 0;
    int y = 0;
    if (be_top(vm) != 2 || !berry_int_arg(vm, 1, &x) || !berry_int_arg(vm, 2, &y))
    {
        push_console_status(vm, ESP_ERR_INVALID_ARG);
        be_return(vm);
    }
    push_console_status(vm, text_console_set_cursor(x, y));
    be_return(vm);
}

int native_console_cursor(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        push_console_status(vm, ESP_ERR_INVALID_ARG);
        be_return(vm);
    }

    int x = 0;
    int y = 0;
    const esp_err_t err = text_console_get_cursor(&x, &y);
    if (err != ESP_OK)
    {
        push_console_status(vm, err);
        be_return(vm);
    }

    be_newobject(vm, "map");
    map_set_number(vm, "x", static_cast<double>(x));
    map_set_number(vm, "y", static_cast<double>(y));
    be_pop(vm, 1);
    be_return(vm);
}

int native_console_size(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        push_console_status(vm, ESP_ERR_INVALID_ARG);
        be_return(vm);
    }

    int cols = 0;
    int rows = 0;
    const esp_err_t err = text_console_get_size(&cols, &rows);
    if (err != ESP_OK)
    {
        push_console_status(vm, err);
        be_return(vm);
    }

    be_newobject(vm, "map");
    map_set_number(vm, "cols", static_cast<double>(cols));
    map_set_number(vm, "rows", static_cast<double>(rows));
    be_pop(vm, 1);
    be_return(vm);
}

int native_console_write(bvm *vm)
{
    const int argc = be_top(vm);
    text_console_color_t color = TEXT_CONSOLE_COLOR_CURRENT;
    if (argc < 1 || argc > 2 || !be_isstring(vm, 1))
    {
        push_console_status(vm, ESP_ERR_INVALID_ARG);
        be_return(vm);
    }
    const esp_err_t color_err = optional_console_color(vm, 2, argc, &color);
    if (color_err != ESP_OK)
    {
        push_console_status(vm, color_err);
        be_return(vm);
    }
    push_console_status(vm, text_console_write_text(be_tostring(vm, 1), color));
    be_return(vm);
}

int native_console_write_at(bvm *vm)
{
    const int argc = be_top(vm);
    int x = 0;
    int y = 0;
    text_console_color_t color = TEXT_CONSOLE_COLOR_CURRENT;
    if (argc < 3 || argc > 4 || !berry_int_arg(vm, 1, &x) || !berry_int_arg(vm, 2, &y) || !be_isstring(vm, 3))
    {
        push_console_status(vm, ESP_ERR_INVALID_ARG);
        be_return(vm);
    }
    const esp_err_t color_err = optional_console_color(vm, 4, argc, &color);
    if (color_err != ESP_OK)
    {
        push_console_status(vm, color_err);
        be_return(vm);
    }
    push_console_status(vm, text_console_write_at(x, y, be_tostring(vm, 3), color));
    be_return(vm);
}

int native_console_clear(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        push_console_status(vm, ESP_ERR_INVALID_ARG);
        be_return(vm);
    }

    int cols = 0;
    int rows = 0;
    const esp_err_t err = text_console_get_size(&cols, &rows);
    if (err != ESP_OK)
    {
        push_console_status(vm, err);
        be_return(vm);
    }
    text_console_clear();
    push_console_status(vm, ESP_OK);
    be_return(vm);
}

int native_console_set_font_size(bvm *vm)
{
    int size = 0;
    if (be_top(vm) != 1 || !berry_int_arg(vm, 1, &size) || size < TEXT_CONSOLE_FONT_SIZE_MIN ||
        size > TEXT_CONSOLE_FONT_SIZE_MAX)
    {
        push_console_status(vm, ESP_ERR_INVALID_ARG);
        be_return(vm);
    }
    push_console_status(vm, text_console_set_font_size(static_cast<uint8_t>(size)));
    be_return(vm);
}

int native_console_font_size(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        push_console_status(vm, ESP_ERR_INVALID_ARG);
        be_return(vm);
    }

    uint8_t size = 0;
    const esp_err_t err = text_console_get_font_size(&size);
    if (err != ESP_OK)
    {
        push_console_status(vm, err);
        be_return(vm);
    }
    be_pushint(vm, size);
    be_return(vm);
}

int native_fs_list(bvm *vm)
{
    char resolved[VFS_PATH_MAX];
    esp_err_t path_error = ESP_OK;
    if (!read_script_path(vm, resolved, &path_error))
    {
        return push_fs_error(vm, path_error);
    }

    static vfs_dir_entry_t entries[VFS_ENTRIES_MAX];
    size_t count = 0;
    const esp_err_t err = vfs_list_dir(resolved, entries, VFS_ENTRIES_MAX, &count);
    be_newobject(vm, "map");
    if (err != ESP_OK)
    {
        map_set_bool(vm, "ok", false);
        map_set_string(vm, "error", esp_err_to_name(err));
        be_pop(vm, 1);
        be_return(vm);
    }

    map_set_bool(vm, "ok", true);
    be_newobject(vm, "list");
    for (size_t index = 0; index < count; ++index)
    {
        be_newobject(vm, "map");
        map_set_string(vm, "name", entries[index].name);
        map_set_number(vm, "size", static_cast<double>(entries[index].size));
        map_set_bool(vm, "is_dir", entries[index].is_dir);
        map_set_number(vm, "mtime", static_cast<double>(entries[index].mtime));
        map_set_number(vm, "ctime", static_cast<double>(entries[index].ctime));
        be_pop(vm, 1);
        be_data_push(vm, -2);
        be_pop(vm, 1);
    }
    be_pushstring(vm, "entries");
    be_pushvalue(vm, -2);
    be_data_insert(vm, -4);
    be_pop(vm, 2);
    be_pop(vm, 1);
    map_set_number(vm, "count", static_cast<double>(count));
    be_pop(vm, 1);
    be_return(vm);
}

int native_fs_read_text(bvm *vm)
{
    char resolved[VFS_PATH_MAX];
    esp_err_t path_error = ESP_OK;
    if (!read_script_path(vm, resolved, &path_error))
    {
        return push_fs_error(vm, path_error);
    }

    bool is_dir = false;
    size_t file_size = 0;
    esp_err_t err = vfs_stat(resolved, &is_dir, &file_size);
    if (err != ESP_OK)
    {
        return push_fs_error(vm, err);
    }
    if (is_dir)
    {
        return push_fs_error(vm, ESP_ERR_INVALID_ARG);
    }
    if (file_size > SCRIPT_TEXT_MAX)
    {
        return push_fs_error(vm, ESP_ERR_INVALID_SIZE);
    }

    char *buffer = static_cast<char *>(std::malloc(file_size + 1));
    if (buffer == nullptr)
    {
        return push_fs_error(vm, ESP_ERR_NO_MEM);
    }

    size_t bytes_read = 0;
    err = file_size == 0 ? ESP_OK : vfs_read_file(resolved, buffer, file_size, &bytes_read);
    if (err != ESP_OK || bytes_read != file_size || std::memchr(buffer, '\0', bytes_read) != nullptr)
    {
        std::free(buffer);
        return push_fs_error(vm, err != ESP_OK ? err : ESP_ERR_INVALID_ARG);
    }

    buffer[bytes_read] = '\0';
    be_newobject(vm, "map");
    map_set_bool(vm, "ok", true);
    map_set_string(vm, "text", buffer);
    std::free(buffer);
    be_pop(vm, 1);
    be_return(vm);
}

int native_fs_write_text(bvm *vm)
{
    char resolved[VFS_PATH_MAX];
    esp_err_t path_error = ESP_OK;
    if (!read_script_path(vm, resolved, &path_error))
    {
        return push_write_status(vm, path_error);
    }
    if (be_top(vm) < 2 || !be_isstring(vm, 2))
    {
        return push_write_status(vm, ESP_ERR_INVALID_ARG);
    }

    const int length = be_strlen(vm, 2);
    if (length < 0 || static_cast<size_t>(length) > SCRIPT_TEXT_MAX)
    {
        return push_write_status(vm, ESP_ERR_INVALID_SIZE);
    }
    const char *text = be_tostring(vm, 2);
    if (text == nullptr)
    {
        return push_write_status(vm, ESP_ERR_INVALID_ARG);
    }
    return push_write_status(vm, vfs_write_file(resolved, text, static_cast<size_t>(length)));
}

int native_fs_read_json(bvm *vm)
{
    char resolved[VFS_PATH_MAX];
    esp_err_t path_error = ESP_OK;
    if (!read_script_path(vm, resolved, &path_error))
    {
        return push_fs_error(vm, path_error);
    }

    bool is_dir = false;
    size_t file_size = 0;
    esp_err_t err = vfs_stat(resolved, &is_dir, &file_size);
    if (err != ESP_OK)
    {
        return push_fs_error(vm, err);
    }
    if (is_dir || file_size == 0)
    {
        return push_fs_error(vm, ESP_ERR_INVALID_ARG);
    }
    if (file_size > SCRIPT_TEXT_MAX)
    {
        return push_fs_error(vm, ESP_ERR_INVALID_SIZE);
    }

    char *buffer = static_cast<char *>(std::malloc(file_size + 1));
    if (buffer == nullptr)
    {
        return push_fs_error(vm, ESP_ERR_NO_MEM);
    }

    size_t bytes_read = 0;
    err = vfs_read_file(resolved, buffer, file_size, &bytes_read);
    if (err != ESP_OK || bytes_read != file_size || std::memchr(buffer, '\0', bytes_read) != nullptr)
    {
        std::free(buffer);
        return push_fs_error(vm, err != ESP_OK ? err : ESP_ERR_INVALID_ARG);
    }

    const char *begin = buffer;
    const char *end = buffer + bytes_read;
    while (begin < end && json_whitespace(*begin))
    {
        ++begin;
    }
    while (end > begin && json_whitespace(end[-1]))
    {
        --end;
    }
    const size_t trimmed_len = static_cast<size_t>(end - begin);
    const bool json_null = trimmed_len == 4 && std::memcmp(begin, "null", 4) == 0;
    if (trimmed_len == 0)
    {
        std::free(buffer);
        return push_fs_error(vm, ESP_ERR_INVALID_ARG);
    }

    be_pushnstring(vm, begin, trimmed_len);
    std::free(buffer);
    const int source_index = be_absindex(vm, -1);
    if (!call_json_method(vm, "load", source_index))
    {
        be_remove(vm, source_index);
        return push_fs_error(vm, ESP_FAIL);
    }
    if (be_isnil(vm, -1) && !json_null)
    {
        be_pop(vm, 2);
        return push_fs_error(vm, ESP_ERR_INVALID_ARG);
    }

    const int value_index = be_absindex(vm, -1);
    be_newobject(vm, "map");
    map_set_bool(vm, "ok", true);
    be_pushstring(vm, "value");
    be_pushvalue(vm, value_index);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
    be_pop(vm, 1);
    be_remove(vm, value_index);
    be_remove(vm, source_index);
    be_return(vm);
}

int native_fs_write_json(bvm *vm)
{
    char resolved[VFS_PATH_MAX];
    esp_err_t path_error = ESP_OK;
    if (!read_script_path(vm, resolved, &path_error))
    {
        return push_write_status(vm, path_error);
    }
    if (be_top(vm) < 2 || !berry_json_value(vm, 2))
    {
        return push_write_status(vm, ESP_ERR_INVALID_ARG);
    }
    if (!call_json_method(vm, "dump", 2) || !be_isstring(vm, -1))
    {
        if (be_top(vm) > 2)
        {
            be_pop(vm, be_top(vm) - 2);
        }
        return push_write_status(vm, ESP_FAIL);
    }

    const int length = be_strlen(vm, -1);
    const char *encoded = be_tostring(vm, -1);
    if (length < 0 || encoded == nullptr || static_cast<size_t>(length) > SCRIPT_TEXT_MAX)
    {
        be_pop(vm, 1);
        return push_write_status(vm, ESP_ERR_INVALID_SIZE);
    }

    const esp_err_t err = vfs_write_file(resolved, encoded, static_cast<size_t>(length));
    be_pop(vm, 1);
    return push_write_status(vm, err);
}

int native_fs_exists(bvm *vm)
{
    char resolved[VFS_PATH_MAX];
    esp_err_t path_error = ESP_OK;
    if (be_top(vm) < 1 || !be_isstring(vm, 1))
    {
        be_raise(vm, "value_error", "path must be a string");
    }
    if (!read_script_path(vm, resolved, &path_error))
    {
        (void)path_error;
        be_pushbool(vm, 0);
        be_return(vm);
    }
    be_pushbool(vm, vfs_exists(resolved) ? 1 : 0);
    be_return(vm);
}

int native_fs_is_dir(bvm *vm)
{
    char resolved[VFS_PATH_MAX];
    esp_err_t path_error = ESP_OK;
    if (be_top(vm) < 1 || !be_isstring(vm, 1))
    {
        be_raise(vm, "value_error", "path must be a string");
    }
    if (!read_script_path(vm, resolved, &path_error))
    {
        (void)path_error;
        be_pushbool(vm, 0);
        be_return(vm);
    }
    be_pushbool(vm, vfs_is_directory(resolved) ? 1 : 0);
    be_return(vm);
}

int native_fs_is_file(bvm *vm)
{
    char resolved[VFS_PATH_MAX];
    esp_err_t path_error = ESP_OK;
    if (be_top(vm) < 1 || !be_isstring(vm, 1))
    {
        be_raise(vm, "value_error", "path must be a string");
    }
    if (!read_script_path(vm, resolved, &path_error))
    {
        (void)path_error;
        be_pushbool(vm, 0);
        be_return(vm);
    }

    bool is_dir = false;
    size_t file_size = 0;
    const bool file = vfs_stat(resolved, &is_dir, &file_size) == ESP_OK && !is_dir;
    be_pushbool(vm, file ? 1 : 0);
    be_return(vm);
}

int native_fs_size(bvm *vm)
{
    char resolved[VFS_PATH_MAX];
    esp_err_t path_error = ESP_OK;
    if (!read_script_path(vm, resolved, &path_error))
    {
        return push_fs_error(vm, path_error);
    }

    bool is_dir = false;
    size_t file_size = 0;
    const esp_err_t err = vfs_stat(resolved, &is_dir, &file_size);
    if (err != ESP_OK)
    {
        return push_fs_error(vm, err);
    }
    if (is_dir)
    {
        return push_fs_error(vm, ESP_ERR_INVALID_ARG);
    }
    if (file_size > static_cast<size_t>(std::numeric_limits<bint>::max()))
    {
        return push_fs_error(vm, ESP_ERR_INVALID_SIZE);
    }

    be_newobject(vm, "map");
    map_set_bool(vm, "ok", true);
    map_set_int(vm, "size", static_cast<bint>(file_size));
    be_pop(vm, 1);
    be_return(vm);
}

// NOLINTNEXTLINE(readability-identifier-naming) - Berry native module attribute table name
static bntvmodobj_t const kConsoleAttrs[] = {
    be_native_module_function("read_line", native_console_read_line),
    be_native_module_function("cancel_input", native_console_cancel_line),
    be_native_module_function("read_key", native_console_read_key),
    be_native_module_function("read_char", native_console_read_char),
    be_native_module_function("move", native_console_move),
    be_native_module_function("cursor", native_console_cursor),
    be_native_module_function("size", native_console_size),
    be_native_module_function("write", native_console_write),
    be_native_module_function("write_at", native_console_write_at),
    be_native_module_function("clear", native_console_clear),
    be_native_module_function("set_font_size", native_console_set_font_size),
    be_native_module_function("font_size", native_console_font_size),
};
} // namespace
// NOLINTNEXTLINE(readability-identifier-naming) - Berry native module symbol required by runtime/module table
extern "C" const bntvmodule_t be_native_module_console = {
    "console",
    kConsoleAttrs,
    sizeof(kConsoleAttrs) / sizeof(kConsoleAttrs[0]),
    nullptr,
};
namespace
{

// NOLINTNEXTLINE(readability-identifier-naming) - Berry native module attribute table name
static bntvmodobj_t const kFsAttrs[] = {
    be_native_module_function("list", native_fs_list),
    be_native_module_function("read_text", native_fs_read_text),
    be_native_module_function("write_text", native_fs_write_text),
    be_native_module_function("read_json", native_fs_read_json),
    be_native_module_function("write_json", native_fs_write_json),
    be_native_module_function("exists", native_fs_exists),
    be_native_module_function("is_dir", native_fs_is_dir),
    be_native_module_function("is_file", native_fs_is_file),
    be_native_module_function("size", native_fs_size),
};
} // namespace
// NOLINTNEXTLINE(readability-identifier-naming) - Berry native module symbol required by runtime/module table
extern "C" const bntvmodule_t be_native_module_fs = {
    "fs",
    kFsAttrs,
    sizeof(kFsAttrs) / sizeof(kFsAttrs[0]),
    nullptr,
};
namespace
{

int load_owned_source(BerrySourceBuffer &source, size_t length, const char *source_name)
{
    // R5: Berry copies/parses the source during be_loadbuffer(); compile_and_release_source
    // releases the owned buffer immediately after compilation (success or failure), keeping
    // peak RAM proportional to source size only until compile completes.
    const int result = compile_and_release_source(source, [source_name, length](const char *buffer)
                                                  { return be_loadbuffer(s_vm, source_name, buffer, length); });
    log_lifecycle_checkpoint("3 Berry compilation complete (source buffer released)");
    return result;
}

esp_err_t execute_source(const char *source, size_t length, const char *source_name)
{
    if (s_vm == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int result = be_loadbuffer(s_vm, source_name, source, length);
    log_lifecycle_checkpoint("3 Berry compilation complete (non-owning source)");

    if (result == 0)
    {
        result = be_pcall(s_vm, 0);
    }

    if (result != 0)
    {
        const char *error = be_tostring(s_vm, -1);
        ESP_LOGE(TAG, "%s: %s", source_name, error != nullptr ? error : "unknown Berry error");
        be_pop(s_vm, 1);
        return ESP_FAIL;
    }

    be_pop(s_vm, 1);
    return ESP_OK;
}

esp_err_t execute_owned_source(BerrySourceBuffer source, size_t length, const char *source_name)
{
    if (s_vm == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int result = load_owned_source(source, length, source_name);
    if (result == 0)
    {
        result = be_pcall(s_vm, 0);
    }

    if (result != 0)
    {
        const char *error = be_tostring(s_vm, -1);
        ESP_LOGE(TAG, "%s: %s", source_name, error != nullptr ? error : "unknown Berry error");
        be_pop(s_vm, 1);
        return ESP_FAIL;
    }

    be_pop(s_vm, 1);
    return ESP_OK;
}

class ScriptPathScope
{
  public:
    explicit ScriptPathScope(const char *virtual_path)
        : pushed_(false)
    {
        if (s_script_depth >= SCRIPT_PATH_STACK || virtual_path == nullptr || virtual_path[0] != '/')
        {
            return;
        }

        const size_t length = std::strlen(virtual_path);
        if (length == 0 || length >= VFS_PATH_MAX)
        {
            return;
        }
        std::memcpy(s_script_paths[s_script_depth], virtual_path, length + 1);
        ++s_script_depth;
        pushed_ = true;
    }

    ~ScriptPathScope()
    {
        if (pushed_)
        {
            --s_script_depth;
        }
    }

    ScriptPathScope(const ScriptPathScope &) = delete;
    ScriptPathScope &operator=(const ScriptPathScope &) = delete;

  private:
    bool pushed_;
};

constexpr char ENTRY_GLOBAL[] = "__cos_entry";

void push_args_list(int argc, char *const argv[])
{
    be_newobject(s_vm, "list");
    for (int index = 0; index < argc; ++index)
    {
        be_pushstring(s_vm, argv[index] != nullptr ? argv[index] : "");
        be_data_push(s_vm, -2);
        be_pop(s_vm, 1);
    }
    be_pop(s_vm, 1);
}

esp_err_t execute_main_entry(BerrySourceBuffer source, size_t length, const char *source_name, int argc,
                             char *const argv[], int *exit_status)
{
    if (exit_status != nullptr)
    {
        *exit_status = 1;
    }
    const ScriptPathScope script_path(source_name);
    (void)script_path;

    esp_err_t err = execute_owned_source(std::move(source), length, source_name);
    if (err != ESP_OK)
    {
        return err;
    }

    be_getglobal(s_vm, ENTRY_GLOBAL);
    const int entry_index = be_absindex(s_vm, -1);
    if (be_isnil(s_vm, entry_index))
    {
        be_pop(s_vm, 1);
        be_getglobal(s_vm, "run");
        if (!be_isfunction(s_vm, -1))
        {
            ESP_LOGE(TAG, "%s: script must define __cos_entry or run(args)", source_name);
            be_pop(s_vm, 1);
            return ESP_ERR_INVALID_ARG;
        }

        push_args_list(argc, argv);
        if (be_pcall(s_vm, 1) != 0)
        {
            const char *error = be_tostring(s_vm, -1);
            ESP_LOGE(TAG, "%s: run(args) failed: %s", source_name, error != nullptr ? error : "unknown Berry error");
            be_pop(s_vm, 2); // error, function/args context
            return ESP_FAIL;
        }
        if (exit_status != nullptr && be_isnil(s_vm, -2))
        {
            *exit_status = 0;
        }
        else if (!be_isint(s_vm, -2) || be_toint(s_vm, -2) < std::numeric_limits<int>::min() ||
                 be_toint(s_vm, -2) > std::numeric_limits<int>::max())
        {
            ESP_LOGE(TAG, "%s: run(args) must return an integer shell status (t[-1]=%s, t[-2]=%s)", source_name,
                     be_typename(s_vm, -1), be_typename(s_vm, -2));
            err = ESP_ERR_INVALID_ARG;
        }
        else if (exit_status != nullptr)
        {
            *exit_status = static_cast<int>(be_toint(s_vm, -2));
        }
        be_pop(s_vm, 2); // args, result(function slot)
        return err;
    }

    const bool is_class = be_isclass(s_vm, entry_index);
    const bool is_instance = be_isinstance(s_vm, entry_index);
    if (!is_class && !is_instance)
    {
        ESP_LOGE(TAG, "%s: __cos_entry must be an app class or instance", source_name);
        be_pop(s_vm, 1);
        return ESP_ERR_INVALID_ARG;
    }

    // UI entrypoints: a class, or an instance with a setup() method.
    bool is_ui_entry = is_class;
    if (is_instance)
    {
        be_getmethod(s_vm, entry_index, "setup");
        is_ui_entry = !be_isnil(s_vm, -1);
        be_pop(s_vm, 1);
    }

    if (is_ui_entry)
    {
        err = berry_runtime_ui_launch(s_vm, entry_index);
        be_pop(s_vm, 1);
        if (err == ESP_OK && exit_status != nullptr)
        {
            *exit_status = 0;
        }
        return err;
    }

    // Console/command entrypoints: instance with run(args).
    be_getmethod(s_vm, entry_index, "run");
    if (be_isnil(s_vm, -1))
    {
        be_pop(s_vm, 2); // nil, entry
        ESP_LOGE(TAG, "%s: __cos_entry must define setup() for UI apps or run(args) for command apps", source_name);
        return ESP_ERR_INVALID_ARG;
    }

    be_pushvalue(s_vm, entry_index); // self
    push_args_list(argc, argv);
    if (be_pcall(s_vm, 2) != 0)
    {
        const char *error = be_tostring(s_vm, -1);
        ESP_LOGE(TAG, "%s: run(args) failed: %s", source_name, error != nullptr ? error : "unknown Berry error");
        be_pop(s_vm, 3); // error, args/self/method context, entry
        return ESP_FAIL;
    }
    if (exit_status != nullptr && be_isnil(s_vm, -3))
    {
        *exit_status = 0;
    }
    else if (!be_isint(s_vm, -3) || be_toint(s_vm, -3) < std::numeric_limits<int>::min() ||
             be_toint(s_vm, -3) > std::numeric_limits<int>::max())
    {
        ESP_LOGE(
            TAG,
            "%s: run(args) must return an integer shell status (top=%d, t[-1]=%s, t[-2]=%s, t[-3]=%s int=%d nil=%d)",
            source_name, be_top(s_vm), be_typename(s_vm, -1), be_typename(s_vm, -2), be_typename(s_vm, -3),
            be_isint(s_vm, -3), be_isnil(s_vm, -3));
        err = ESP_ERR_INVALID_ARG;
    }
    else if (exit_status != nullptr)
    {
        *exit_status = static_cast<int>(be_toint(s_vm, -3));
    }

    be_pop(s_vm, 4); // args, self, result(method slot), entry
    return err;
}

esp_err_t invoke_app_method(const char *method, bool with_dimensions)
{
    be_getglobal(s_vm, APP_GLOBAL);
    if (!be_isinstance(s_vm, -1))
    {
        be_pop(s_vm, 1);
        return ESP_ERR_INVALID_STATE;
    }

    be_getmethod(s_vm, -1, method);
    if (be_isnil(s_vm, -1))
    {
        be_pop(s_vm, 2);
        return ESP_OK;
    }

    be_pushvalue(s_vm, -2);
    int argument_count = 1;
    if (with_dimensions)
    {
        be_pushint(s_vm, 0);
        be_pushint(s_vm, display_get_width());
        be_pushint(s_vm, display_get_height());
        argument_count += 3;
    }

    if (be_pcall(s_vm, argument_count) != 0)
    {
        const char *error = be_tostring(s_vm, -1);
        ESP_LOGE(TAG, "Berry app %s failed: %s", method, error != nullptr ? error : "unknown Berry error");
        be_pop(s_vm, argument_count + 2);
        return ESP_FAIL;
    }

    be_pop(s_vm, argument_count + 2);
    return ESP_OK;
}

bool has_active_app()
{
    be_getglobal(s_vm, APP_GLOBAL);
    const bool active = be_isinstance(s_vm, -1);
    be_pop(s_vm, 1);
    return active;
}

esp_err_t close_active_app()
{
    const bool had_active_app = has_active_app();
    esp_err_t err = cancel_pending_console_input();
    if (had_active_app)
    {
        const esp_err_t teardown_err = invoke_app_method("teardown", false);
        if (err == ESP_OK)
        {
            err = teardown_err;
        }
    }

    log_lifecycle_checkpoint(had_active_app ? "6 Before app teardown cleanup"
                                            : "6 Before app-state cleanup (no active app)");
    ui::stop();

    berry_ui_end_app();
    clear_app_class_global(s_app_class_global_name);
    s_app_class_global_name = nullptr;
    be_pushnil(s_vm);
    be_setglobal(s_vm, APP_GLOBAL);
    be_pop(s_vm, 1);
    be_pushnil(s_vm);
    be_setglobal(s_vm, MODULE_CACHE_GLOBAL);
    be_pop(s_vm, 1);
    collect_berry_garbage();
    if (had_active_app)
    {
        const esp_err_t visible_err = text_console_set_visible(true);
        if (visible_err != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not restore console display: %s", esp_err_to_name(visible_err));
            if (err == ESP_OK)
            {
                err = visible_err;
            }
        }
    }
    s_app_script_set = false;
    return err;
}

void discard_app()
{
    berry_ui_end_app();
    ui::stop();
}

extern "C" esp_err_t berry_runtime_ui_launch(bvm *vm, int index)
{
    if (vm == nullptr || vm != s_vm)
    {
        return ESP_ERR_INVALID_STATE;
    }

    be_pushvalue(vm, index);
    const bool is_class = be_isclass(vm, -1);
    const bool is_instance = be_isinstance(vm, -1);
    if (!is_class && !is_instance)
    {
        be_pop(vm, 1);
        ESP_LOGE(TAG, "ui.launch: argument must be an app class or instance");
        return ESP_ERR_INVALID_ARG;
    }

    const char *app_class_name = nullptr;
    bool app_class_is_global = false;
    if (is_class)
    {
        app_class_name = be_classname(vm, -1);
        if (app_class_name != nullptr)
        {
            be_pushvalue(vm, -1);
            be_getglobal(vm, app_class_name);
            app_class_is_global = be_iseq(vm);
            be_pop(vm, 2);
        }
    }

    esp_err_t err = close_active_app();
    if (err != ESP_OK)
    {
        be_pop(vm, 1);
        return err;
    }
    err = text_console_release_scrollback();
    if (err != ESP_OK)
    {
        be_pop(vm, 1);
        clear_app_class_global(app_class_is_global ? app_class_name : nullptr);
        return err;
    }

    berry_ui_begin_app(vm);

    if (is_class)
    {
        if (be_pcall(vm, 0) != 0)
        {
            const char *error = be_tostring(vm, -1);
            ESP_LOGE(TAG, "ui.launch: app construction failed: %s", error != nullptr ? error : "unknown Berry error");
            be_pop(vm, 1);
            discard_app();
            clear_app_class_global(app_class_is_global ? app_class_name : nullptr);
            collect_berry_garbage();
            return ESP_FAIL;
        }
        log_lifecycle_checkpoint("4 Berry app constructed");
    }

    if (!be_isinstance(vm, -1))
    {
        be_pop(vm, 1);
        discard_app();
        clear_app_class_global(app_class_is_global ? app_class_name : nullptr);
        collect_berry_garbage();
        ESP_LOGE(TAG, "ui.launch: expected an app instance");
        return ESP_ERR_INVALID_ARG;
    }

    err = text_console_set_visible(false);
    const bool console_hidden = err == ESP_OK;
    if (err != ESP_OK)
    {
        be_pop(vm, 1);
        discard_app();
        clear_app_class_global(app_class_is_global ? app_class_name : nullptr);
        collect_berry_garbage();
        return err;
    }

    be_setglobal(vm, APP_GLOBAL);
    be_pop(vm, 1);
    s_app_class_global_name = app_class_is_global ? app_class_name : nullptr;

    err = invoke_app_method("setup", true);
    if (err == ESP_OK)
    {
        err = ui::start() ? ESP_OK : ESP_ERR_NO_MEM;
        log_lifecycle_checkpoint("5 Berry app setup complete");
    }
    if (err != ESP_OK)
    {
        discard_app();
        clear_app_class_global(s_app_class_global_name);
        s_app_class_global_name = nullptr;
        be_pushnil(vm);
        be_setglobal(vm, APP_GLOBAL);
        be_pop(vm, 1);
        collect_berry_garbage();
        if (console_hidden)
        {
            const esp_err_t visible_err = text_console_set_visible(true);
            if (visible_err != ESP_OK)
            {
                ESP_LOGE(TAG, "Could not restore console display: %s", esp_err_to_name(visible_err));
            }
        }
        return err;
    }
    if (has_active_app())
    {
        retain_app_script_path();
    }
    return ESP_OK;
}

esp_err_t read_virtual_file(const char *virtual_path,
                            BerrySourceBuffer *source, // NOLINT(readability-non-const-parameter) - output object
                                                       // reset/populated by this function
                            size_t *length)
{
    // R5: stat first, then allocate the short-lived source buffer before opening/reading.
    // This avoids stdio/VFS read-path allocations fragmenting the heap before the large
    // contiguous buffer is secured (observed: 8224-byte breakout source failed when
    // largest block dropped from 13824 to 6400 between pre-load checkpoint and calloc).
    // The buffer is released immediately after be_loadbuffer() via compile_and_release_source.
    source->reset();
    if (length == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *length = 0;

    bool is_dir = false;
    size_t file_size = 0;
    esp_err_t err = vfs_stat(virtual_path, &is_dir, &file_size);
    if (err != ESP_OK)
    {
        return err;
    }
    if (is_dir)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (file_size >= std::numeric_limits<size_t>::max())
    {
        return ESP_ERR_NO_MEM;
    }

    *length = file_size;
    // Short-lived buffer: released after be_loadbuffer() (compile_and_release_source).
    // R5 hardware: vfs_stat()/VFS can fragment default heap before allocation (8224-byte
    // buffer failed after checkpoint showed largest=18432). Prefer a capability that still
    // has a contiguous block; fall back broadly. heap_caps allocations are free()-compatible.
    const size_t need = file_size + 1;
    const size_t def_largest = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    const size_t int_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    char *buffer = nullptr;
    if (int_largest >= need && def_largest < need)
    {
        buffer = static_cast<char *>(heap_caps_malloc(need, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }
    if (buffer == nullptr)
    {
        buffer = static_cast<char *>(std::malloc(need));
    }
    if (buffer == nullptr)
    {
        buffer = static_cast<char *>(heap_caps_malloc(need, MALLOC_CAP_8BIT));
    }
    if (buffer == nullptr)
    {
        buffer = static_cast<char *>(heap_caps_malloc(need, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }
    if (buffer == nullptr)
    {
        ESP_LOGW(TAG, "Source buffer alloc failed (%u bytes): def_free=%u def_largest=%u int_largest=%u after stat",
                 static_cast<unsigned>(need), static_cast<unsigned>(esp_get_free_heap_size()),
                 static_cast<unsigned>(def_largest), static_cast<unsigned>(int_largest));
    }
    source->reset(buffer);
    if (!*source)
    {
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = 0;
    err = file_size == 0 ? ESP_OK : vfs_read_file(virtual_path, source->get(), file_size, &bytes_read);
    if (err != ESP_OK || bytes_read != file_size)
    {
        source->reset();
        return err != ESP_OK ? err : ESP_FAIL;
    }

    source->get()[file_size] = '\0';
    return ESP_OK;
}

bool resolve_module_path(const char *specifier,
                         char *resolved, // NOLINT(readability-non-const-parameter) - output path buffer
                         size_t resolved_size)
{
    char composed[VFS_PATH_MAX];
    const esp_err_t composed_err =
        vfs_compose_script_path(current_script_path(), specifier, composed, sizeof(composed));
    if (composed_err != ESP_OK)
    {
        return false;
    }

    auto try_path = [resolved, resolved_size](const char *candidate) -> bool
    {
        char real_path[VFS_PATH_MAX];
        if (vfs_resolve_path(candidate, real_path, sizeof(real_path)) != ESP_OK)
        {
            return false;
        }
        const size_t len = std::strlen(candidate);
        if (len + 1 > resolved_size)
        {
            return false; // NOLINT(readability-simplify-boolean-expr) - explicit guard return
        }
        std::memcpy(resolved, candidate, len + 1);
        return true;
    };

    const size_t len = std::strlen(composed);
    if (len >= 3 && std::strcmp(composed + len - 3, ".ts") == 0)
    {
        char candidate[VFS_PATH_MAX];
        const int written = std::snprintf(candidate, sizeof(candidate), "%.*s.be", static_cast<int>(len - 3), composed);
        if (written > 0 && static_cast<size_t>(written) < sizeof(candidate) && try_path(candidate))
        {
            return true;
        }
    }
    else if (len >= 4 && std::strcmp(composed + len - 4, ".tsx") == 0)
    {
        char candidate[VFS_PATH_MAX];
        const int written = std::snprintf(candidate, sizeof(candidate), "%.*s.be", static_cast<int>(len - 4), composed);
        if (written > 0 && static_cast<size_t>(written) < sizeof(candidate) && try_path(candidate))
        {
            return true;
        }
    }
    else if (len >= 3 && std::strcmp(composed + len - 3, ".be") == 0)
    {
        if (try_path(composed))
        {
            return true;
        }
    }
    else
    {
        char candidate[VFS_PATH_MAX];
        const int written = std::snprintf(candidate, sizeof(candidate), "%s.be", composed);
        if (written > 0 && static_cast<size_t>(written) < sizeof(candidate) && try_path(candidate))
        {
            return true;
        }
    }

    return try_path(composed);
}

void ensure_module_cache(bvm *vm)
{
    if (be_getglobal(vm, MODULE_CACHE_GLOBAL))
    {
        be_pop(vm, 1);
        return;
    }
    be_pop(vm, 1);
    be_newobject(vm, "map");
    be_setglobal(vm, MODULE_CACHE_GLOBAL);
    be_pop(vm, 1);
}

void push_map_instance(bvm *vm)
{
    be_getbuiltin(vm, "map");
    be_call(vm, 0);
    be_pop(vm, 0);
}

void copy_map_entries(bvm *vm, int src_index, int dst_map_index)
{
    src_index = be_absindex(vm, src_index);
    dst_map_index = be_absindex(vm, dst_map_index);
    if (!be_pushiter(vm, src_index))
    {
        return;
    }
    const int iter_index = be_absindex(vm, -1);
    while (be_iter_hasnext(vm, src_index))
    {
        be_iter_next(vm, src_index);
        // stack: [..., dst_map, ..., src_map, ..., iter, key, value]
        const int key_index = be_absindex(vm, -2);
        const int value_index = be_absindex(vm, -1);
        be_pushvalue(vm, key_index);
        be_pushvalue(vm, value_index);
        be_data_insert(vm, dst_map_index);
        be_pop(vm, 2);
        be_pop(vm, 2);
    }
    be_pop(vm, 1);
    (void)iter_index;
}

int native_cos_module(bvm *vm)
{
    const int argc = be_top(vm);
    push_map_instance(vm);
    const int inst_index = be_absindex(vm, -1);
    be_getmember(vm, inst_index, ".p");
    const int map_index = be_absindex(vm, -1);
    be_pushstring(vm, "__cos_module__");
    be_pushbool(vm, 1);
    be_data_insert(vm, map_index);
    be_pop(vm, 2);
    be_pushstring(vm, "default");
    if (argc >= 1)
    {
        be_pushvalue(vm, 1);
    }
    else
    {
        be_pushnil(vm);
    }
    be_data_insert(vm, map_index);
    be_pop(vm, 2);
    for (int index = 2; index + 1 <= argc; index += 2)
    {
        if (!be_isstring(vm, index))
        {
            continue;
        }
        be_pushvalue(vm, index);
        be_pushvalue(vm, index + 1);
        be_data_insert(vm, map_index);
        be_pop(vm, 2);
    }
    be_pop(vm, 1);
    be_return(vm);
}

const char *resolve_cos_module_name(const char *specifier)
{
    if (specifier == nullptr)
    {
        return nullptr;
    }
    const char *name = nullptr;
    const char cos_prefix[] = "@cos/";
    const char berry_prefix[] = "@berry/";
    if (std::strncmp(specifier, cos_prefix, sizeof(cos_prefix) - 1) == 0)
    {
        name = specifier + (sizeof(cos_prefix) - 1);
    }
    else if (std::strncmp(specifier, berry_prefix, sizeof(berry_prefix) - 1) == 0)
    {
        name = specifier + (sizeof(berry_prefix) - 1);
    }
    else
    {
        return nullptr;
    }

    if (std::strcmp(name, "network") == 0)
    {
        return "net";
    }
    if (std::strcmp(name, "filesystem") == 0)
    {
        return "fs";
    }
    return name;
}

int native_cos_import(bvm *vm)
{
    if (be_top(vm) < 1 || !be_isstring(vm, 1))
    {
        be_raise(vm, "value_error", "module specifier must be a string");
    }

    const char *specifier = be_tostring(vm, 1);
    if (specifier == nullptr)
    {
        be_raise(vm, "value_error", "module specifier must be a string");
    }

    const char *cos_module = resolve_cos_module_name(specifier);
    const bool relative = specifier[0] == '.' && (specifier[1] == '/' || (specifier[1] == '.' && specifier[2] == '/'));

    if (cos_module == nullptr && !relative)
    {
        char message[128];
        std::snprintf(message, sizeof(message), "unsupported module specifier: %s", specifier);
        be_raise(vm, "import_error", message);
    }

    if (relative && current_script_path() == nullptr)
    {
        be_raise(vm, "import_error", "relative imports need a script file");
    }

    const char *cache_key = specifier;
    char resolved[VFS_PATH_MAX];
    if (relative)
    {
        if (!resolve_module_path(specifier, resolved, sizeof(resolved)))
        {
            char message[VFS_PATH_MAX + 32];
            std::snprintf(message, sizeof(message), "module not found: %s", specifier);
            be_raise(vm, "import_error", message);
        }
        cache_key = resolved;
    }

    ensure_module_cache(vm);
    be_getglobal(vm, MODULE_CACHE_GLOBAL);
    const int cache_index = be_absindex(vm, -1);
    be_pushstring(vm, cache_key);
    if (be_getindex(vm, cache_index) && !be_isnil(vm, -1))
    {
        be_remove(vm, cache_index);
        be_return(vm);
    }
    be_pop(vm, 1);

    if (cos_module != nullptr)
    {
        if (!be_getmodule(vm, cos_module))
        {
            be_pop(vm, 1);
            char message[128];
            std::snprintf(message, sizeof(message), "module not found: %s", specifier);
            be_raise(vm, "import_error", message);
        }
        const int module_index = be_absindex(vm, -1);
        push_map_instance(vm);
        const int inst_index = be_absindex(vm, -1);
        be_getmember(vm, inst_index, ".p");
        const int dst = be_absindex(vm, -1);
        be_pushstring(vm, "__cos_module__");
        be_pushbool(vm, 1);
        be_data_insert(vm, dst);
        be_pop(vm, 2);
        be_pushstring(vm, "default");
        be_pushvalue(vm, module_index);
        be_data_insert(vm, dst);
        be_pop(vm, 2);
        be_pop(vm, 1);               // pop .p
        be_remove(vm, module_index); // remove native module, keep wrapper
        be_pushstring(vm, cache_key);
        be_pushvalue(vm, -2);
        be_data_insert(vm, cache_index);
        be_pop(vm, 2);
        be_remove(vm, cache_index);
        (void)inst_index;
        be_return(vm);
    }

    BerrySourceBuffer source(nullptr, &std::free);
    size_t length = 0; // NOLINT(misc-const-correctness) - written via read_virtual_file output parameter
    const esp_err_t read_err = read_virtual_file(resolved, &source, &length);
    if (read_err != ESP_OK)
    {
        be_pop(vm, 1);
        char message[VFS_PATH_MAX + 32];
        std::snprintf(message, sizeof(message), "could not read module: %s", resolved);
        be_raise(vm, "import_error", message);
    }

    const ScriptPathScope script_path(resolved);
    (void)script_path;
    int result = load_owned_source(source, length, resolved);
    if (result == 0)
    {
        result = be_pcall(vm, 0);
    }
    if (result != 0)
    {
        const char *error = be_tostring(vm, -1);
        ESP_LOGE(TAG, "Module import failed (%s): %s", resolved, error != nullptr ? error : "unknown Berry error");
        be_pop(vm, 2);
        char message[VFS_PATH_MAX + 48];
        std::snprintf(message, sizeof(message), "module import failed: %s", specifier);
        be_raise(vm, "import_error", message);
    }

    int value_index = be_absindex(vm, -1);
    bool is_descriptor = false;
    if (be_ismapinstance(vm, value_index))
    {
        be_getmember(vm, value_index, ".p");
        const int data = be_absindex(vm, -1);
        be_pushstring(vm, "__cos_module__");
        is_descriptor = be_getindex(vm, data) && be_isbool(vm, -1) && be_tobool(vm, -1);
        be_pop(vm, 2);
    }
    if (!is_descriptor && be_ismap(vm, value_index))
    {
        be_pushstring(vm, "__cos_module__");
        is_descriptor = be_getindex(vm, value_index) && be_isbool(vm, -1) && be_tobool(vm, -1);
        be_pop(vm, 1);
        if (is_descriptor)
        {
            push_map_instance(vm);
            const int inst_index = be_absindex(vm, -1);
            be_getmember(vm, inst_index, ".p");
            const int dst = be_absindex(vm, -1);
            copy_map_entries(vm, value_index, dst);
            be_pop(vm, 1);
            be_remove(vm, value_index);
            value_index = be_absindex(vm, -1);
            (void)inst_index;
        }
    }
    if (!is_descriptor)
    {
        push_map_instance(vm);
        const int inst_index = be_absindex(vm, -1);
        be_getmember(vm, inst_index, ".p");
        const int dst = be_absindex(vm, -1);
        be_pushstring(vm, "__cos_module__");
        be_pushbool(vm, 1);
        be_data_insert(vm, dst);
        be_pop(vm, 2);
        be_pushstring(vm, "default");
        be_pushvalue(vm, value_index);
        be_data_insert(vm, dst);
        be_pop(vm, 2);
        be_pop(vm, 1);
        be_remove(vm, value_index);
        value_index = be_absindex(vm, -1);
        (void)inst_index;
    }

    be_pushstring(vm, cache_key);
    be_pushvalue(vm, value_index);
    be_data_insert(vm, cache_index);
    be_remove(vm, cache_index);
    be_return(vm);
}
} // namespace

namespace
{
static int native_cos_lazy_member(bvm *vm)
{
    // Internal helper for R6 lazy proxy globals. Called as
    // cos_lazy_member(module_name, attr_name) from a Berry closure installed as
    // the proxy module's `member` hook. Loads the real native module on first
    // access, replaces the global, and returns the requested attribute.
    if (be_top(vm) < 2 || !be_isstring(vm, 1) || !be_isstring(vm, 2))
    {
        be_return_nil(vm);
    }

    const char *module_name = be_tostring(vm, 1);
    const char *attr = be_tostring(vm, 2);
    if (module_name == nullptr || attr == nullptr)
    {
        be_return_nil(vm);
    }

    if (!be_getmodule(vm, module_name))
    {
        char message[80];
        std::snprintf(message, sizeof(message), "on-demand module load failed: %s", module_name);
        be_raise(vm, "import_error", message);
    }

    // Stack: [..., real_module]
    be_pushvalue(vm, -1);
    be_setglobal(vm, module_name);

    if (!be_getmember(vm, -1, attr))
    {
        // Do not mask a real module miss as nil for normal attributes; raise to
        // preserve Berry's attribute_error semantics where possible.
        be_pop(vm, 1);
        char message[96];
        std::snprintf(message, sizeof(message), "module '%s' has no member '%s'", module_name, attr);
        be_raise(vm, "attribute_error", message);
    }

    be_remove(vm, -2); // keep attribute value
    be_return(vm);
}

static void install_lazy_module_global(bvm *vm, const char *name)
{
    // Install a tiny proxy module whose `member` hook is a Berry closure.
    // Using a closure (not a native function) avoids be_module_attr's low-level
    // be_dofunc/ret_native interaction that is subtle for native member hooks.
    // The closure calls cos_lazy_member(name, attr) which loads the real native
    // module on demand and replaces this proxy global.
    const int base = be_top(vm);
    char source[128];
    std::snprintf(source, sizeof(source), "return def(k) return cos_lazy_member('%s', k) end", name);

    be_newmodule(vm); // stack: module

    if (be_loadbuffer(vm, name, source, std::strlen(source)) != 0 || be_pcall(vm, 0) != 0)
    {
        if (be_isstring(vm, -1))
        {
            ESP_LOGW(TAG, "Lazy proxy init failed for '%s': %s", name, be_tostring(vm, -1));
        }
        else
        {
            ESP_LOGW(TAG, "Lazy proxy init failed for '%s'", name);
        }
        be_pop(vm, be_top(vm) - base);
        return;
    }

    if (be_top(vm) - base != 2 || !be_isfunction(vm, -1))
    {
        be_pop(vm, be_top(vm) - base);
        return;
    }
    // stack: module, closure
    be_setmember(vm, -2, "member");
    be_pop(vm, 1); // be_setmember does not pop value in Berry 1.x API
    // stack: module
    be_setglobal(vm, name);
    be_pop(vm, 1);
}
} // namespace

extern "C" esp_err_t berry_runtime_init(void)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (s_vm != nullptr)
    {
        return ESP_OK;
    }

    s_vm = be_vm_new();
    if (s_vm == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create Berry VM");
        return ESP_ERR_NO_MEM;
    }
    s_gc_step_rate = BERRY_GC_STEP_RATE_DEFAULT;

    be_regfunc(s_vm, "log", native_log);
    be_regfunc(s_vm, "action", native_action);
    be_regfunc(s_vm, "cos_import", native_cos_import);
    be_regfunc(s_vm, "cos_module", native_cos_module);
    be_regfunc(s_vm, "cos_lazy_member", native_cos_lazy_member);

    // R6 (2026-11-08): COS native modules are not eagerly loaded at Berry
    // startup. `console`, `fs`, `net`, and `ui` remain in be_modtab.c and are
    // loaded on demand via Berry `import <module>`, `be_getmodule()`,
    // `cos_import('@cos/*')` (TS-compiled COS apps/scripts), or first bare-global
    // attribute access through lightweight lazy proxy globals installed below.
    // `json` is also on-demand (R1): it stays in the native module table and is
    // loaded by berry_runtime_ensure_json() for FS JSON / net.fetch JSON paths.
    //
    // Compatibility: existing bare-global Berry usage (`ui.*`, `net.*`, `fs.*`,
    // `console.*`) still works, but the first attribute access loads the real
    // module. Scripts that do not use a module avoid its load/allocation. New TS
    // code should prefer explicit `import ... from '@cos/*'`.
    install_lazy_module_global(s_vm, "console");
    install_lazy_module_global(s_vm, "fs");
    install_lazy_module_global(s_vm, "net");
    install_lazy_module_global(s_vm, "ui");

    ui::set_execution_guard(berry_runtime_lock, berry_runtime_unlock);
    ui::set_tick_callback(berry_ui_tick);
    ui::set_exit_callback(close_app_from_ui);

    log_lifecycle_checkpoint("1 Berry VM initialized");

    return ESP_OK;
}

extern "C" void berry_runtime_deinit(void)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (s_vm != nullptr)
    {
        cancel_pending_console_input();
        close_active_app();
        be_vm_delete(s_vm);
        s_vm = nullptr;
    }
}

extern "C" esp_err_t berry_runtime_eval(const char *source, const char *source_name)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (source == nullptr || source_name == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_vm == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return execute_source(source, std::strlen(source), source_name);
}

extern "C" esp_err_t berry_runtime_run_file(const char *virtual_path, int argc, char *const argv[], int *exit_status)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (virtual_path == nullptr || argc < 0 || (argc > 0 && argv == nullptr))
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (int index = 0; index < argc; ++index)
    {
        if (argv[index] == nullptr)
        {
            return ESP_ERR_INVALID_ARG;
        }
    }

    if (s_vm == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (exit_status != nullptr)
    {
        *exit_status = 1;
    }

    BerrySourceBuffer source(nullptr, &std::free);
    size_t length = 0; // NOLINT(misc-const-correctness) - written via read_virtual_file output parameter
    const esp_err_t err = read_virtual_file(virtual_path, &source, &length);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NO_MEM)
        {
            ESP_LOGW(TAG, "Could not allocate Berry source buffer (%u bytes); free=%u, largest=%u, internal_largest=%u",
                     static_cast<unsigned>(length + 1), static_cast<unsigned>(esp_get_free_heap_size()),
                     static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)),
                     static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));
        }
        return err;
    }
    ESP_LOGI(TAG, "Loaded Berry file (%u bytes)", static_cast<unsigned>(length));
    log_lifecycle_checkpoint("2 Berry source loaded");
    return execute_main_entry(std::move(source), length, virtual_path, argc, argv, exit_status);
}

extern "C" esp_err_t berry_runtime_open_app(const char *virtual_path)
{
    int exit_status = 1;
    const esp_err_t err = berry_runtime_run_file(virtual_path, 0, nullptr, &exit_status);
    if (err != ESP_OK)
    {
        return err;
    }
    return exit_status == 0 ? ESP_OK : ESP_FAIL;
}

extern "C" esp_err_t berry_runtime_close_app(void)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (s_vm == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return close_active_app();
}

extern "C" bool berry_runtime_handle_key_event(const keyboard_event_t *event)
{
    if (event == nullptr)
    {
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (s_vm == nullptr || !has_active_app())
    {
        return false;
    }
    berry_ui_set_key_state(event->keycode, event->pressed);
    return true;
}

extern "C" void berry_runtime_set_console_input_provider(berry_console_read_line_fn read_line,
                                                         berry_console_cancel_line_fn cancel_line,
                                                         berry_console_read_key_fn read_key,
                                                         berry_console_read_key_fn read_char)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    s_console_read_line = read_line;
    s_console_cancel_line = cancel_line;
    s_console_read_key = read_key;
    s_console_read_char = read_char;
}

std::recursive_mutex &
berry_runtime_mutex() // NOLINT(misc-use-internal-linkage) - external runtime API used across translation units

{
    return s_vm_mutex;
}

void berry_runtime_lock()
{
    s_vm_mutex.lock();
}

void berry_runtime_unlock()
{
    s_vm_mutex.unlock();
}

extern "C" esp_err_t berry_runtime_get_memory_info(berry_runtime_memory_info_t *info)
{
    if (info == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);

    info->initialized = (s_vm != nullptr);
    info->gc_managed_bytes = s_vm != nullptr ? be_gc_memcount(s_vm) : 0;
    info->gc_step_rate = s_vm != nullptr ? s_gc_step_rate : BERRY_GC_STEP_RATE_DEFAULT;
    info->heap_free = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    info->heap_total = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    info->heap_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    info->heap_min_free = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    info->internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    info->internal_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    info->internal_min_free = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);

    return ESP_OK;
}

extern "C" esp_err_t berry_runtime_gc_collect(size_t *before_bytes, size_t *after_bytes)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (s_vm == nullptr)
    {
        if (before_bytes != nullptr)
        {
            *before_bytes = 0;
        }
        if (after_bytes != nullptr)
        {
            *after_bytes = 0;
        }
        return ESP_ERR_INVALID_STATE;
    }

    const size_t before = be_gc_memcount(s_vm);
    be_gc_collect(s_vm);
    const size_t after = be_gc_memcount(s_vm);
    if (before_bytes != nullptr)
    {
        *before_bytes = before;
    }
    if (after_bytes != nullptr)
    {
        *after_bytes = after;
    }
    return ESP_OK;
}

extern "C" esp_err_t berry_runtime_set_gc_step_rate(int rate)
{
    if (rate < 100 || rate > 500)
    {
        return ESP_ERR_INVALID_ARG;
    }

    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (s_vm == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    be_gc_setsteprate(s_vm, rate);
    s_gc_step_rate = rate;
    ESP_LOGI(TAG, "Berry GC step rate set to %d (default %d); use only for R5 experiments until benchmarked", rate,
             BERRY_GC_STEP_RATE_DEFAULT);
    return ESP_OK;
}