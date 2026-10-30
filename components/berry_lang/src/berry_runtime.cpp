#include "berry_runtime.h"

#include "berry.h"
#include "berry_internal.h"
#include "display.h"
#include "filesystem.h"
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
void berry_ui_tick(void);

namespace
{
constexpr char TAG[] = "berry";
bvm *s_vm = nullptr;
constexpr char APP_GLOBAL[] = "_cos_berry_app";
std::recursive_mutex s_vm_mutex;

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
    esp_err_t err = ESP_OK;
    if (had_active_app)
    {
        err = invoke_app_method("teardown", false);
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
