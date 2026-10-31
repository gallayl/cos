#include "berry_runtime.h"

#include "berry.h"
#include "berry_internal.h"
#include "display.h"
#include "filesystem.h"
#include "network.h"
#include "text_console.h"
#include "ui.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
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
        be_return(vm);
    }

    const char *path = be_tostring(vm, 1);
    if (!valid_virtual_path(path))
    {
        be_newobject(vm, "map");
        map_set_bool(vm, "ok", false);
        map_set_string(vm, "error", esp_err_to_name(ESP_ERR_INVALID_ARG));
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
        be_data_push(vm, -2);
        be_pop(vm, 1);
    }
    be_pushstring(vm, "entries");
    be_pushvalue(vm, -2);
    be_data_insert(vm, -4);
    be_pop(vm, 2);
    be_pop(vm, 1);
    map_set_number(vm, "count", static_cast<double>(count));
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
    ui::stop();
    berry_ui_end_app();
    be_pushnil(s_vm);
    be_setglobal(s_vm, APP_GLOBAL);
    be_pop(s_vm, 1);
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
    berry_ui_begin_app(s_vm);

    int result = be_loadbuffer(s_vm, source_name, source, length);
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
        return ESP_FAIL;
    }

    if (!be_isclass(s_vm, -1))
    {
        be_pop(s_vm, 1);
        discard_app();
        return require_app ? ESP_ERR_INVALID_ARG : ESP_OK;
    }

    if (be_pcall(s_vm, 0) != 0)
    {
        const char *error = be_tostring(s_vm, -1);
        ESP_LOGE(TAG, "%s: app construction failed: %s", source_name, error != nullptr ? error : "unknown Berry error");
        be_pop(s_vm, 1);
        discard_app();
        return ESP_FAIL;
    }

    be_setglobal(s_vm, APP_GLOBAL);
    be_pop(s_vm, 1);
    esp_err_t err = invoke_app_method("setup", true);
    if (err == ESP_OK)
    {
        err = text_console_set_visible(false);
        if (err == ESP_OK && !ui::start())
        {
            esp_err_t visible_err = text_console_set_visible(true);
            if (visible_err != ESP_OK)
            {
                ESP_LOGE(TAG, "Could not restore console display: %s", esp_err_to_name(visible_err));
            }
            err = ESP_ERR_NO_MEM;
        }
    }
    if (err != ESP_OK)
    {
        discard_app();
        be_pushnil(s_vm);
        be_setglobal(s_vm, APP_GLOBAL);
        be_pop(s_vm, 1);
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

    *length = static_cast<size_t>(file_size);
    *source = static_cast<char *>(std::malloc(*length + 1));
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
    (*source)[*length] = '\0';
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

    char *source = nullptr;
    size_t length = 0;
    esp_err_t err = read_virtual_file(virtual_path, &source, &length);
    if (err != ESP_OK)
    {
        return err;
    }
    err = execute_app_source(source, length, virtual_path, false);
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

    char *source = nullptr;
    size_t length = 0;
    esp_err_t err = read_virtual_file(virtual_path, &source, &length);
    if (err != ESP_OK)
    {
        return err;
    }

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
