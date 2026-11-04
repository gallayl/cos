#include "berry_runtime.h"

#include "berry.h"
#include "berry_internal.h"
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

void berry_ui_register(bvm *vm);
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
std::recursive_mutex s_vm_mutex;
berry_console_read_line_fn s_console_read_line = nullptr;
berry_console_cancel_line_fn s_console_cancel_line = nullptr;
bool s_console_line_pending = false;
int s_console_line_token;
const char *s_app_class_global_name = nullptr;

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

bool valid_virtual_path(const char *path)
{
    if (path == nullptr || path[0] != '/' || std::strlen(path) >= VFS_PATH_MAX)
    {
        return false;
    }

    const bool mounted_path = std::strcmp(path, "/") == 0 || std::strcmp(path, "/flash") == 0 ||
                              std::strncmp(path, "/flash/", 7) == 0 || std::strcmp(path, "/sdcard") == 0 ||
                              std::strncmp(path, "/sdcard/", 8) == 0;
    if (!mounted_path)
    {
        return false;
    }

    const char *segment = path;
    while (*segment != '\0')
    {
        while (*segment == '/')
        {
            ++segment;
        }
        const char *end = std::strchr(segment, '/');
        const size_t length = end != nullptr ? static_cast<size_t>(end - segment) : std::strlen(segment);
        if (length == 2 && segment[0] == '.' && segment[1] == '.')
        {
            return false;
        }
        segment += length;
    }
    return true;
}

int native_fs_list(bvm *vm)
{
    if (be_top(vm) < 1 || !be_isstring(vm, 1))
    {
        be_newobject(vm, "map");
        map_set_bool(vm, "ok", false);
        map_set_string(vm, "error", esp_err_to_name(ESP_ERR_INVALID_ARG));
        be_pop(vm, 1);
        be_return(vm);
    }

    const char *path = be_tostring(vm, 1);
    if (!valid_virtual_path(path))
    {
        be_newobject(vm, "map");
        map_set_bool(vm, "ok", false);
        map_set_string(vm, "error", esp_err_to_name(ESP_ERR_INVALID_ARG));
        be_pop(vm, 1);
        be_return(vm);
    }

    static vfs_dir_entry_t entries[VFS_ENTRIES_MAX];
    size_t count = 0;
    const esp_err_t err = vfs_list_dir(path, entries, VFS_ENTRIES_MAX, &count);
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

int native_net_ping(bvm *vm)
{
    const char *host = nullptr;
    uint32_t timeout_ms = 1000;
    esp_err_t err = ESP_OK;
    network_ping_result_t result = {};

    if (be_top(vm) < 1 || !be_isstring(vm, 1))
    {
        err = ESP_ERR_INVALID_ARG;
    }
    else
    {
        host = be_tostring(vm, 1);
        if (be_top(vm) >= 2)
        {
            if (!be_isint(vm, 2))
            {
                err = ESP_ERR_INVALID_ARG;
            }
            else
            {
                const bint requested_timeout = be_toint(vm, 2);
                if (requested_timeout < NETWORK_PING_TIMEOUT_MIN_MS || requested_timeout > NETWORK_PING_TIMEOUT_MAX_MS)
                {
                    err = ESP_ERR_INVALID_ARG;
                }
                else
                {
                    timeout_ms = static_cast<uint32_t>(requested_timeout);
                }
            }
        }
        if (err == ESP_OK)
        {
            err = network_ping(host, timeout_ms, &result);
        }
    }

    be_newobject(vm, "map");
    map_set_bool(vm, "ok", err == ESP_OK);
    map_set_string(vm, "host", host != nullptr ? host : "");
    map_set_string(vm, "address", result.address);
    map_set_number(vm, "rtt_ms", static_cast<double>(result.rtt_ms));
    map_set_string(vm, "error", err == ESP_OK ? "" : esp_err_to_name(err));
    be_pop(vm, 1);
    be_return(vm);
}

void register_console_module(bvm *vm)
{
    be_newmodule(vm);
    be_setname(vm, -1, "console");
    be_pushntvfunction(vm, native_console_read_line);
    be_setmember(vm, -2, "read_line");
    be_pop(vm, 1);
    be_pushntvfunction(vm, native_console_cancel_line);
    be_setmember(vm, -2, "cancel_input");
    be_pop(vm, 1);
    be_setglobal(vm, "console");
    be_pop(vm, 1);
}

void register_filesystem_module(bvm *vm)
{
    be_newmodule(vm);
    be_setname(vm, -1, "fs");
    be_pushntvfunction(vm, native_fs_list);
    be_setmember(vm, -2, "list");
    be_pop(vm, 1);
    be_setglobal(vm, "fs");
    be_pop(vm, 1);
}

void register_network_module(bvm *vm)
{
    be_newmodule(vm);
    be_setname(vm, -1, "net");
    be_pushntvfunction(vm, native_net_ping);
    be_setmember(vm, -2, "ping");
    be_pop(vm, 1);
    be_setglobal(vm, "net");
    be_pop(vm, 1);
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

void restore_command_main(int saved_main_index)
{
    be_pushvalue(s_vm, saved_main_index);
    be_setglobal(s_vm, "main");
    be_pop(s_vm, 1);
    be_pop(s_vm, 1);
}

esp_err_t execute_command_source(const char *source, size_t length, const char *source_name, int argc,
                                 char *const argv[], int *exit_status)
{
    const int saved_main_index = be_top(s_vm) + 1;
    be_getglobal(s_vm, "main");
    be_pushnil(s_vm);
    be_setglobal(s_vm, "main");
    be_pop(s_vm, 1);

    esp_err_t err = execute_source(source, length, source_name);
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
    return err;
}

void discard_app()
{
    berry_ui_end_app();
    ui::stop();
}

esp_err_t execute_app_source(const char *source, size_t length, const char *source_name, bool require_app)
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
    log_heap_snapshot("Before Berry source execution");
    berry_ui_begin_app(s_vm);

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
    return err;
}

esp_err_t read_virtual_file(const char *virtual_path, char **source, size_t *length)
{
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
    *source = static_cast<char *>(std::calloc(*length + 1, sizeof(char)));
    if (*source == nullptr)
    {
        std::fclose(file);
        return ESP_ERR_NO_MEM;
    }
    const size_t bytes_read = std::fread(*source, 1, *length, file);
    const bool read_error = std::ferror(file) != 0;
    std::fclose(file);
    if (read_error || bytes_read != *length)
    {
        std::free(*source);
        *source = nullptr;
        return ESP_FAIL;
    }
    return ESP_OK;
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
    register_console_module(s_vm);
    register_filesystem_module(s_vm);
    register_network_module(s_vm);
    berry_ui_register(s_vm);
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
    char *source = nullptr;
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
    err = execute_app_source(source, length, virtual_path, false);
    std::free(source);
    return err;
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
    char *source = nullptr;
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

    err = execute_command_source(source, length, virtual_path, argc, argv, exit_status);
    std::free(source);
    return err;
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
    char *source = nullptr;
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

    err = execute_app_source(source, length, virtual_path, true);
    std::free(source);
    return err;
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
                                                         berry_console_cancel_line_fn cancel_line)
{
    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);
    s_console_read_line = read_line;
    s_console_cancel_line = cancel_line;
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

extern "C" esp_err_t berry_runtime_get_memory_info(
    berry_runtime_memory_info_t *info
)
{
    if (info == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    std::lock_guard<std::recursive_mutex> lock(s_vm_mutex);

    info->initialized = (s_vm != nullptr);
    info->gc_managed_bytes =
        s_vm != nullptr ? be_gc_memcount(s_vm) : 0;

    return ESP_OK;
}