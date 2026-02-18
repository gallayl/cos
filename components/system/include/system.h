#pragma once

#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Snapshot of system information. */
    typedef struct
    {
        const char *idf_version; /**< ESP-IDF SDK version string. */
        uint32_t cpu_freq_mhz;   /**< CPU frequency in MHz. */
        uint32_t flash_size;     /**< External flash size in bytes. */
        size_t free_heap;        /**< Current free heap in bytes. */
        size_t min_free_heap;    /**< Minimum free heap since boot. */
        size_t total_heap;       /**< Total heap capacity in bytes. */
        size_t max_alloc_block;  /**< Largest allocatable block in bytes. */
    } system_info_t;

    /**
     * @brief Initialize the system component and register shell commands.
     *
     * Registers the info, memory, restart, and uptime commands.
     */
    esp_err_t system_init(void);

    /**
     * @brief Populate a system_info_t with current system state.
     *
     * @param info Output struct to fill.
     * @return ESP_OK on success, ESP_ERR_INVALID_ARG if info is NULL.
     */
    esp_err_t system_get_info(system_info_t *info);

    /**
     * @brief Format the uptime (time since boot) as a human-readable string.
     *
     * @param buf  Output buffer (e.g. "1h 23m 45s").
     * @param len  Buffer capacity (>= 24 recommended).
     * @return ESP_OK on success, ESP_ERR_INVALID_SIZE if buffer is too small.
     */
    esp_err_t system_format_uptime(char *buf, size_t len);

    /** @brief Restart the device immediately. */
    void system_restart(void);

#ifdef __cplusplus
}
#endif
