#pragma once

#include "esp_err.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Boot profile identifiers (v1). */
    typedef enum
    {
        BOOT_PROFILE_DEFAULT = 0,
        BOOT_PROFILE_LOW_MEMORY_OFFLINE,
    } boot_profile_t;

    /** Canonical profile name strings. */
#define BOOT_PROFILE_NAME_DEFAULT "default"
#define BOOT_PROFILE_NAME_LOW_MEMORY_OFFLINE "low-memory-offline"

    /**
     * @brief Read boot profile state from NVS and compute effective profile for this boot.
     *
     * Must be called after NVS is initialized. Safe to call early in app_main before
     * service lifecycle boot. Missing/invalid/corrupt stored value falls back to default.
     */
    esp_err_t boot_profile_early_init(void);

    /** @brief Initialize boot profile component and register shell command. */
    esp_err_t boot_profile_init(void);

    /** @brief Get effective profile for the current boot. */
    boot_profile_t boot_profile_get_effective(void);

    /** @brief Get profile intended for next boot based on current stored value/fallback. */
    boot_profile_t boot_profile_get_next(void);

    /**
     * @brief Get stored raw profile value.
     *
     * @param out Buffer receiving NUL-terminated stored value or "default" when absent/invalid.
     * @param out_size Buffer size.
     * @param has_stored Set to true if a valid stored value exists, false if missing/invalid/fallback.
     * @return ESP_OK on success, ESP_ERR_INVALID_ARG on bad args, ESP_ERR_INVALID_SIZE if buffer too small.
     */
    esp_err_t boot_profile_get_stored(char *out, size_t out_size, bool *has_stored);

    /** @brief Persist profile selection for next boot. */
    esp_err_t boot_profile_set_stored(const char *name);

    /** @brief Clear stored profile selection (next boot uses default). */
    esp_err_t boot_profile_clear_stored(void);

    /** @brief Convert profile enum to canonical name. */
    const char *boot_profile_name(boot_profile_t profile);

    /** @brief Parse canonical profile name. Returns true if valid. */
    bool boot_profile_parse(const char *name, boot_profile_t *out);

    /** @brief True if active boot profile allows Wi-Fi service to be started/enabled. */
    bool boot_profile_allows_wifi(void);

    /** @brief True if active boot profile allows network diagnostics service. */
    bool boot_profile_allows_network_diagnostics(void);

    /** @brief True if active boot profile allows Bluetooth stack/radio operations. */
    bool boot_profile_allows_bluetooth_stack(void);

    /** @brief True if active boot profile allows HTTP server service. */
    bool boot_profile_allows_http_server(void);

    /** @brief True if active boot profile allows WebSocket service. */
    bool boot_profile_allows_websocket(void);

    /** @brief True if active boot profile allows time sync service. */
    bool boot_profile_allows_time_sync(void);

#ifdef __cplusplus
}
#endif
