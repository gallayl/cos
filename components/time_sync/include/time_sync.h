#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize SNTP time synchronization and register shell commands.
     *
     * Configures SNTP in poll mode (pool.ntp.org) and sets the local
     * timezone. Does not block waiting for synchronization.
     */
    esp_err_t time_sync_init(void);

    /**
     * @brief Check whether the system clock has been synchronized via SNTP.
     * @return true if time has been set (year > 2020), false otherwise.
     */
    bool time_sync_is_synced(void);

    /** @brief Get the current UNIX epoch time. */
    time_t time_sync_get_epoch(void);

    /**
     * @brief Format the current UTC time as ISO 8601.
     *
     * @param buf Output buffer (e.g. "2026-02-18T14:30:00Z").
     * @param len Size of the buffer (>= 21 recommended).
     * @return ESP_OK on success, ESP_ERR_INVALID_SIZE if buffer is too small.
     */
    esp_err_t time_sync_get_utc(char *buf, size_t len);

    /**
     * @brief Format the current local time as ISO 8601 with UTC offset.
     *
     * @param buf Output buffer (e.g. "2026-02-18T15:30:00+01:00").
     * @param len Size of the buffer (>= 26 recommended).
     * @return ESP_OK on success, ESP_ERR_INVALID_SIZE if buffer is too small.
     */
    esp_err_t time_sync_get_local(char *buf, size_t len);

#ifdef __cplusplus
}
#endif
