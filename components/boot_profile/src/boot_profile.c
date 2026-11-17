#include "boot_profile.h"

#include "esp_log.h"
#include "nvs.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "boot_profile";

void boot_profile_register_commands(void);

#define BOOT_NVS_NAMESPACE "boot"
#define BOOT_NVS_KEY_PROFILE "profile"
#define BOOT_PROFILE_MAX_LEN 32

static boot_profile_t s_effective_profile = BOOT_PROFILE_DEFAULT;
static bool s_has_valid_stored = false;
static char s_stored_raw[BOOT_PROFILE_MAX_LEN] = "";
static bool s_early_initialized = false;
static bool s_commands_registered = false;

const char *boot_profile_name(boot_profile_t profile)
{
    switch (profile)
    {
    case BOOT_PROFILE_LOW_MEMORY_OFFLINE:
        return BOOT_PROFILE_NAME_LOW_MEMORY_OFFLINE;
    case BOOT_PROFILE_DEFAULT:
    default:
        return BOOT_PROFILE_NAME_DEFAULT;
    }
}

bool boot_profile_parse(const char *name, boot_profile_t *out)
{
    if (name == NULL)
    {
        return false;
    }

    if (strcmp(name, BOOT_PROFILE_NAME_DEFAULT) == 0)
    {
        if (out != NULL)
        {
            *out = BOOT_PROFILE_DEFAULT;
        }
        return true;
    }

    if (strcmp(name, BOOT_PROFILE_NAME_LOW_MEMORY_OFFLINE) == 0)
    {
        if (out != NULL)
        {
            *out = BOOT_PROFILE_LOW_MEMORY_OFFLINE;
        }
        return true;
    }

    return false;
}

static void set_default_state(void)
{
    s_effective_profile = BOOT_PROFILE_DEFAULT;
    s_has_valid_stored = false;
    s_stored_raw[0] = '\0';
}

esp_err_t boot_profile_early_init(void)
{
    set_default_state();

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(BOOT_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        s_early_initialized = true;
        ESP_LOGI(TAG, "No stored boot profile, using default");
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        s_early_initialized = true;
        ESP_LOGW(TAG, "Failed to open boot profile NVS (%s), using default", esp_err_to_name(err));
        return ESP_OK;
    }

    /* Try string first, then blob compatibility. */
    size_t len = sizeof(s_stored_raw);
    err = nvs_get_str(handle, BOOT_NVS_KEY_PROFILE, s_stored_raw, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        len = sizeof(s_stored_raw);
        err = nvs_get_blob(handle, BOOT_NVS_KEY_PROFILE, s_stored_raw, &len);
        if (err == ESP_OK)
        {
            if (len >= sizeof(s_stored_raw))
            {
                len = sizeof(s_stored_raw) - 1;
            }
            s_stored_raw[len] = '\0';
        }
    }

    nvs_close(handle);

    if (err != ESP_OK)
    {
        if (err != ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGW(TAG, "Failed to read stored boot profile (%s), using default", esp_err_to_name(err));
        }
        set_default_state();
        s_early_initialized = true;
        return ESP_OK;
    }

    /* Trim accidental trailing whitespace/NUL padding for robustness. */
    size_t raw_len = strnlen(s_stored_raw, sizeof(s_stored_raw));
    while (raw_len > 0 && (s_stored_raw[raw_len - 1] == '\0' || s_stored_raw[raw_len - 1] == ' ' ||
                           s_stored_raw[raw_len - 1] == '\r' || s_stored_raw[raw_len - 1] == '\n' ||
                           s_stored_raw[raw_len - 1] == '\t'))
    {
        s_stored_raw[--raw_len] = '\0';
    }

    boot_profile_t parsed = BOOT_PROFILE_DEFAULT;
    if (boot_profile_parse(s_stored_raw, &parsed))
    {
        s_has_valid_stored = true;
        s_effective_profile = parsed;
        ESP_LOGI(TAG, "Stored boot profile: %s (effective: %s)", s_stored_raw, boot_profile_name(s_effective_profile));
    }
    else
    {
        ESP_LOGW(TAG, "Invalid/corrupt stored boot profile '%s', using default", s_stored_raw);
        set_default_state();
    }

    s_early_initialized = true;
    return ESP_OK;
}

esp_err_t boot_profile_init(void)
{
    if (!s_early_initialized)
    {
        (void)boot_profile_early_init();
    }

    if (!s_commands_registered)
    {
        /* Requires esp_console to be initialized (e.g. by shell). Best-effort for recovery when console is available.
         */
        boot_profile_register_commands();
        s_commands_registered = true;
    }

    ESP_LOGI(TAG, "Boot profile command ready (effective=%s, next=%s)", boot_profile_name(s_effective_profile),
             boot_profile_name(boot_profile_get_next()));
    return ESP_OK;
}

boot_profile_t boot_profile_get_effective(void)
{
    return s_effective_profile;
}

boot_profile_t boot_profile_get_next(void)
{
    if (s_has_valid_stored)
    {
        boot_profile_t parsed = BOOT_PROFILE_DEFAULT;
        if (boot_profile_parse(s_stored_raw, &parsed))
        {
            return parsed;
        }
    }
    return BOOT_PROFILE_DEFAULT;
}

esp_err_t boot_profile_get_stored(char *out, size_t out_size, bool *has_stored)
{
    if (out == NULL || out_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *value = s_has_valid_stored ? s_stored_raw : BOOT_PROFILE_NAME_DEFAULT;
    int written = snprintf(out, out_size, "%s", value);
    if (written < 0 || (size_t)written >= out_size)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    if (has_stored != NULL)
    {
        *has_stored = s_has_valid_stored;
    }
    return ESP_OK;
}

esp_err_t boot_profile_set_stored(const char *name)
{
    boot_profile_t parsed = BOOT_PROFILE_DEFAULT;
    if (!boot_profile_parse(name, &parsed))
    {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(BOOT_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_str(handle, BOOT_NVS_KEY_PROFILE, boot_profile_name(parsed));
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }
    nvs_close(handle);

    if (err == ESP_OK)
    {
        snprintf(s_stored_raw, sizeof(s_stored_raw), "%s", boot_profile_name(parsed));
        s_has_valid_stored = true;
    }
    return err;
}

esp_err_t boot_profile_clear_stored(void)
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(BOOT_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        s_has_valid_stored = false;
        s_stored_raw[0] = '\0';
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_erase_key(handle, BOOT_NVS_KEY_PROFILE);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = ESP_OK;
    }
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }
    nvs_close(handle);

    if (err == ESP_OK)
    {
        s_has_valid_stored = false;
        s_stored_raw[0] = '\0';
    }
    return err;
}

bool boot_profile_allows_wifi(void)
{
    return s_effective_profile == BOOT_PROFILE_DEFAULT;
}

bool boot_profile_allows_network_diagnostics(void)
{
    return s_effective_profile == BOOT_PROFILE_DEFAULT;
}

bool boot_profile_allows_bluetooth_stack(void)
{
    return s_effective_profile == BOOT_PROFILE_DEFAULT;
}

bool boot_profile_allows_http_server(void)
{
    return s_effective_profile == BOOT_PROFILE_DEFAULT;
}

bool boot_profile_allows_websocket(void)
{
    return s_effective_profile == BOOT_PROFILE_DEFAULT;
}

bool boot_profile_allows_time_sync(void)
{
    return s_effective_profile == BOOT_PROFILE_DEFAULT;
}
