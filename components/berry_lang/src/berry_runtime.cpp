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
constexpr int kScriptPathStack = 4;
constexpr size_t kScriptTextMax = 4096;
char s_script_paths[kScriptPathStack][VFS_PATH_MAX];
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

void log_heap_snapshot(const char *phase)
{
    ESP_LOGI(TAG, "%s: free=%u bytes, minimum=%u bytes, largest block=%u bytes", phase,
             static_cast<unsigned>(esp_get_free_heap_size()), static_cast<unsigned>(esp_get_minimum_free_heap_size()),
             static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
}

void collect_berry_garbage()
{
    const size_t before = be_gc_memcount(s_vm);
    be_gc_collect(s_vm);
    const size_t after = be_gc_memcount(s_vm);
    const size_t reclaimed = before > after ? before - after : 0;
    ESP_LOGI(TAG, "Berry GC: %u bytes in use (%u reclaimed)", static_cast<unsigned>(after),
             static_cast<unsigned>(reclaimed));
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
    esp_err_t err = berry_runtime_close_app();
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
    esp_err_t err = esp_console_run(command, &command_result);
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

    be_pushbool(vm, true);
    be_return(vm);
}

int native_console_cancel_line(bvm *vm)
{
    if (!s_console_line_pending || s_console_cancel_line == nullptr)
    {
        be_pushbool(vm, false);
        be_return(vm);
    }

    const esp_err_t err = s_console_cancel_line(&s_console_line_token);
    if (err != ESP_OK)
    {
        be_pushstring(vm, esp_err_to_name(err));
        be_return(vm);
    }
    be_pushbool(vm, true);
    be_return(vm);
}

void map_set_bool(bvm *vm, const char *key, bool value)
{
    be_pushstring(vm, key);
    be_pushbool(vm, value);
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
        be_pushbool(vm, true);
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
        be_pushbool(vm, true);
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
        be_pushbool(vm, true);
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
        be_pushbool(vm, true);
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
        be_pop(vm, 1);
        be_data_push(vm, -2);
        be_pop(vm, 1);
    }
    be_pop(vm, 1);
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
    if (file_size > kScriptTextMax)
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
    if (length < 0 || static_cast<size_t>(length) > kScriptTextMax)
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
    if (file_size > kScriptTextMax)
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
    if (length < 0 || encoded == nullptr || static_cast<size_t>(length) > kScriptTextMax)
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
        be_pushbool(vm, false);
        be_return(vm);
    }
    be_pushbool(vm, vfs_exists(resolved));
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
        be_pushbool(vm, false);
        be_return(vm);
    }
    be_pushbool(vm, vfs_is_directory(resolved));
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
        be_pushbool(vm, false);
        be_return(vm);
    }

    bool is_dir = false;
    size_t file_size = 0;
    const bool file = vfs_stat(resolved, &is_dir, &file_size) == ESP_OK && !is_dir;
    be_pushbool(vm, file);
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

static bntvmodobj_t const console_attrs[] = {
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
extern "C" const bntvmodule_t be_native_module_console = {
    "console",
    console_attrs,
    sizeof(console_attrs) / sizeof(console_attrs[0]),
    nullptr,
};
namespace
{

static bntvmodobj_t const fs_attrs[] = {
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
extern "C" const bntvmodule_t be_native_module_fs = {
    "fs",
    fs_attrs,
    sizeof(fs_attrs) / sizeof(fs_attrs[0]),
    nullptr,
};
namespace
{

int load_owned_source(BerrySourceBuffer &source, size_t length, const char *source_name)
{
    const int result = compile_and_release_source(source, [source_name, length](const char *buffer)
                                                  { return be_loadbuffer(s_vm, source_name, buffer, length); });
    log_heap_snapshot("After Berry compilation");
    return result;
}

esp_err_t execute_source(const char *source, size_t length, const char *source_name)
{
    if (s_vm == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int result = be_loadbuffer(s_vm, source_name, source, length);
    log_heap_snapshot("After Berry compilation");

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
        if (s_script_depth >= kScriptPathStack || virtual_path == nullptr || virtual_path[0] != '/')
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

void restore_command_main(int saved_main_index)
{
    be_pushvalue(s_vm, saved_main_index);
    be_setglobal(s_vm, "main");
    be_pop(s_vm, 1);
    be_pop(s_vm, 1);
}

esp_err_t execute_command_source(BerrySourceBuffer source, size_t length, const char *source_name, int argc,
                                 char *const argv[], int *exit_status)
{
    const ScriptPathScope script_path(source_name);
    (void)script_path;
    const int saved_main_index = be_top(s_vm) + 1;
    be_getglobal(s_vm, "main");
    be_pushnil(s_vm);
    be_setglobal(s_vm, "main");
    be_pop(s_vm, 1);

    esp_err_t err = execute_owned_source(std::move(source), length, source_name);
    if (err == ESP_OK)
    {
        be_getglobal(s_vm, "main");
        if (!be_isfunction(s_vm, -1))
        {
            ESP_LOGE(TAG, "%s: script must define main(args)", source_name);
            be_pop(s_vm, 1);
            err = ESP_ERR_INVALID_ARG;
        }
        else
        {
            be_newobject(s_vm, "list");
            for (int index = 0; index < argc; ++index)
            {
                be_pushstring(s_vm, argv[index]);
                be_data_push(s_vm, -2);
                be_pop(s_vm, 1);
            }
            be_pop(s_vm, 1);

            if (be_pcall(s_vm, 1) != 0)
            {
                const char *error = be_tostring(s_vm, -1);
                ESP_LOGE(TAG, "%s: command failed: %s", source_name, error != nullptr ? error : "unknown Berry error");
                be_pop(s_vm, 1);
                err = ESP_FAIL;
            }
            else if (!be_isint(s_vm, -2) || be_toint(s_vm, -2) < std::numeric_limits<int>::min() ||
                     be_toint(s_vm, -2) > std::numeric_limits<int>::max())
            {
                ESP_LOGE(TAG, "%s: main(args) must return an integer shell status", source_name);
                err = ESP_ERR_INVALID_ARG;
            }
            else
            {
                *exit_status = static_cast<int>(be_toint(s_vm, -2));
            }
        }
    }

    be_pop(s_vm, be_top(s_vm) - saved_main_index);
    restore_command_main(saved_main_index);
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

    log_heap_snapshot("Before app teardown");
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
        esp_err_t visible_err = text_console_set_visible(true);
        if (visible_err != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not restore console display: %s", esp_err_to_name(visible_err));
            if (err == ESP_OK)
            {
                err = visible_err;
            }
        }
    }
    log_heap_snapshot("After app teardown and GC");
    s_app_script_set = false;
    return err;
}

void discard_app()
{
    berry_ui_end_app();
    ui::stop();
}

esp_err_t execute_app_source(BerrySourceBuffer source, size_t length, const char *source_name, bool require_app)
{
    if (s_vm == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t close_err = close_active_app();
    if (close_err != ESP_OK)
    {
        return close_err;
    }
    const esp_err_t history_err = text_console_release_scrollback();
    if (history_err != ESP_OK)
    {
        return history_err;
    }
    const ScriptPathScope script_path(source_name);
    (void)script_path;
    log_heap_snapshot("Before Berry source execution");
    berry_ui_begin_app(s_vm);

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
        discard_app();
        collect_berry_garbage();
        return ESP_FAIL;
    }

    if (!be_isclass(s_vm, -1))
    {
        be_pop(s_vm, 1);
        discard_app();
        collect_berry_garbage();
        return require_app ? ESP_ERR_INVALID_ARG : ESP_OK;
    }

    const char *app_class_name = be_classname(s_vm, -1);
    bool app_class_is_global = false;
    if (app_class_name != nullptr)
    {
        be_pushvalue(s_vm, -1);
        be_getglobal(s_vm, app_class_name);
        app_class_is_global = be_iseq(s_vm);
        be_pop(s_vm, 2);
    }

    esp_err_t err = text_console_release_scrollback();
    if (err == ESP_OK)
    {
        err = text_console_set_visible(false);
    }
    const bool console_hidden = err == ESP_OK;
    if (err != ESP_OK)
    {
        be_pop(s_vm, 1);
        discard_app();
        clear_app_class_global(app_class_is_global ? app_class_name : nullptr);
        collect_berry_garbage();
        return err;
    }

    if (be_pcall(s_vm, 0) != 0)
    {
        const char *error = be_tostring(s_vm, -1);
        ESP_LOGE(TAG, "%s: app construction failed: %s", source_name, error != nullptr ? error : "unknown Berry error");
        be_pop(s_vm, 1);
        discard_app();
        if (console_hidden)
        {
            const esp_err_t visible_err = text_console_set_visible(true);
            if (visible_err != ESP_OK)
            {
                ESP_LOGE(TAG, "Could not restore console display: %s", esp_err_to_name(visible_err));
            }
        }
        clear_app_class_global(app_class_is_global ? app_class_name : nullptr);
        collect_berry_garbage();
        return ESP_FAIL;
    }

    be_setglobal(s_vm, APP_GLOBAL);
    be_pop(s_vm, 1);
    s_app_class_global_name = app_class_is_global ? app_class_name : nullptr;

    err = invoke_app_method("setup", true);
    if (err == ESP_OK)
    {
        err = ui::start() ? ESP_OK : ESP_ERR_NO_MEM;
        log_heap_snapshot("After app setup");
    }
    if (err != ESP_OK)
    {
        discard_app();
        clear_app_class_global(s_app_class_global_name);
        s_app_class_global_name = nullptr;
        be_pushnil(s_vm);
        be_setglobal(s_vm, APP_GLOBAL);
        be_pop(s_vm, 1);
        collect_berry_garbage();
        if (console_hidden)
        {
            const esp_err_t visible_err = text_console_set_visible(true);
            if (visible_err != ESP_OK)
            {
                ESP_LOGE(TAG, "Could not restore console display: %s", esp_err_to_name(visible_err));
            }
        }
    }
    if (err == ESP_OK && has_active_app())
    {
        retain_app_script_path();
    }
    return err;
}

esp_err_t read_virtual_file(const char *virtual_path, BerrySourceBuffer *source, size_t *length)
{
    source->reset();
    char real_path[VFS_PATH_MAX];
    esp_err_t err = vfs_resolve_path(virtual_path, real_path, sizeof(real_path));
    if (err != ESP_OK)
    {
        return err;
    }

    FILE *file = std::fopen(real_path, "rb");
    if (file == nullptr)
    {
        return ESP_ERR_NOT_FOUND;
    }
    if (std::fseek(file, 0, SEEK_END) != 0)
    {
        std::fclose(file);
        return ESP_FAIL;
    }
    const long file_size = std::ftell(file);
    if (file_size < 0 || std::fseek(file, 0, SEEK_SET) != 0)
    {
        std::fclose(file);
        return ESP_FAIL;
    }

    if (static_cast<std::uintmax_t>(file_size) >= std::numeric_limits<size_t>::max())
    {
        std::fclose(file);
        return ESP_ERR_NO_MEM;
    }

    *length = static_cast<size_t>(file_size);
    source->reset(static_cast<char *>(std::calloc(*length + 1, sizeof(char))));
    if (!*source)
    {
        std::fclose(file);
        return ESP_ERR_NO_MEM;
    }
    const size_t bytes_read = std::fread(source->get(), 1, *length, file);
    const bool read_error = std::ferror(file) != 0;
    std::fclose(file);
    if (read_error || bytes_read != *length)
    {
        source->reset();
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool resolve_module_path(const char *specifier, char *resolved, size_t resolved_size)
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
            return false;
        }
        std::memcpy(resolved, candidate, len + 1);
        return true;
    };

    const size_t len = std::strlen(composed);
    if (len >= 3 && std::strcmp(composed + len - 3, ".ts") == 0)
    {
        char candidate[VFS_PATH_MAX];
        if (len + 1 < sizeof(candidate))
        {
            std::memcpy(candidate, composed, len - 3);
            std::memcpy(candidate + len - 3, ".be", 4);
            if (try_path(candidate))
            {
                return true;
            }
        }
    }
    else if (len >= 4 && std::strcmp(composed + len - 4, ".tsx") == 0)
    {
        char candidate[VFS_PATH_MAX];
        if (len + 1 < sizeof(candidate))
        {
            std::memcpy(candidate, composed, len - 4);
            std::memcpy(candidate + len - 4, ".be", 4);
            if (try_path(candidate))
            {
                return true;
            }
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
        if (len + 3 < sizeof(candidate))
        {
            std::memcpy(candidate, composed, len);
            std::memcpy(candidate + len, ".be", 4);
            if (try_path(candidate))
            {
                return true;
            }
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
    be_pushbool(vm, true);
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
        be_pushbool(vm, true);
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
    size_t length = 0;
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
        be_pushbool(vm, true);
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
bool install_module_global(bvm *vm, const char *name)
{
    if (!be_getmodule(vm, name))
    {
        ESP_LOGW(TAG, "Could not load native module '%s'", name);
        return false;
    }
    be_setglobal(vm, name);
    be_pop(vm, 1);
    return true;
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

    be_regfunc(s_vm, "log", native_log);
    be_regfunc(s_vm, "action", native_action);
    be_regfunc(s_vm, "cos_import", native_cos_import);
    be_regfunc(s_vm, "cos_module", native_cos_module);

    // Backward-compatible globals for existing scripts; modules are also
    // available lazily via `import` / `cos_import('@cos/*')`.
    // `json` is intentionally not preloaded at startup (R1 / 2026-11-06 P2):
    // it stays in the native module table for `import json` and is loaded on
    // demand by berry_runtime_ensure_json() from FS JSON / net.fetch JSON paths.
    install_module_global(s_vm, "console");
    install_module_global(s_vm, "fs");
    install_module_global(s_vm, "net");
    install_module_global(s_vm, "ui");

    ui::set_execution_guard(berry_runtime_lock, berry_runtime_unlock);
    ui::set_tick_callback(berry_ui_tick);
    ui::set_exit_callback(close_app_from_ui);

    log_heap_snapshot("Berry VM initialized");

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

extern "C" esp_err_t berry_runtime_run_file(const char *virtual_path)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (virtual_path == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_vm == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = text_console_release_scrollback();
    if (err != ESP_OK)
    {
        return err;
    }
    log_heap_snapshot("Before Berry file load");
    BerrySourceBuffer source(nullptr, &std::free);
    size_t length = 0;
    err = read_virtual_file(virtual_path, &source, &length);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NO_MEM)
        {
            ESP_LOGW(TAG, "Could not allocate Berry source buffer (%u bytes); free=%u, largest block=%u",
                     static_cast<unsigned>(length + 1), static_cast<unsigned>(esp_get_free_heap_size()),
                     static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        }
        return err;
    }
    ESP_LOGI(TAG, "Loaded Berry file (%u bytes)", static_cast<unsigned>(length));
    log_heap_snapshot("After Berry file load");
    return execute_app_source(std::move(source), length, virtual_path, false);
}

extern "C" esp_err_t berry_runtime_exec_file(const char *virtual_path, int argc, char *const argv[], int *exit_status)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (virtual_path == nullptr || argc < 0 || (argc > 0 && argv == nullptr) || exit_status == nullptr)
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

    *exit_status = 1;
    BerrySourceBuffer source(nullptr, &std::free);
    size_t length = 0;
    esp_err_t err = read_virtual_file(virtual_path, &source, &length);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NO_MEM)
        {
            ESP_LOGW(TAG, "Could not allocate Berry command source buffer (%u bytes); free=%u, largest block=%u",
                     static_cast<unsigned>(length + 1), static_cast<unsigned>(esp_get_free_heap_size()),
                     static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        }
        return err;
    }

    return execute_command_source(std::move(source), length, virtual_path, argc, argv, exit_status);
}

extern "C" esp_err_t berry_runtime_open_app(const char *virtual_path)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    if (virtual_path == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_vm == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = text_console_release_scrollback();
    if (err != ESP_OK)
    {
        return err;
    }
    log_heap_snapshot("Before Berry app load");
    BerrySourceBuffer source(nullptr, &std::free);
    size_t length = 0;
    err = read_virtual_file(virtual_path, &source, &length);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NO_MEM)
        {
            ESP_LOGW(TAG, "Could not allocate Berry app source buffer (%u bytes); free=%u, largest block=%u",
                     static_cast<unsigned>(length + 1), static_cast<unsigned>(esp_get_free_heap_size()),
                     static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
        }
        return err;
    }
    ESP_LOGI(TAG, "Loaded Berry app file (%u bytes)", static_cast<unsigned>(length));
    log_heap_snapshot("After Berry app load");

    return execute_app_source(std::move(source), length, virtual_path, true);
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

std::recursive_mutex &berry_runtime_mutex()
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