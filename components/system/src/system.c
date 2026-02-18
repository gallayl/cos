#include "system.h"
#include "sdkconfig.h"

#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include <stdio.h>

static const char *const TAG = "system";

void system_register_commands(void);

esp_err_t system_init(void)
{
    system_register_commands();
    ESP_LOGI(TAG, "System component initialized");
    return ESP_OK;
}

esp_err_t system_get_info(system_info_t *info)
{
    if (info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    info->idf_version = esp_get_idf_version();
    info->cpu_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    info->flash_size = flash_size;

    info->free_heap = esp_get_free_heap_size();
    info->min_free_heap = esp_get_minimum_free_heap_size();
    info->total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    info->max_alloc_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    return ESP_OK;
}

esp_err_t system_format_uptime(char *buf, size_t len)
{
    if (buf == NULL || len < 4)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    int64_t us = esp_timer_get_time();
    uint32_t total_secs = (uint32_t)(us / 1000000);

    uint32_t days = total_secs / 86400;
    uint32_t hours = (total_secs % 86400) / 3600;
    uint32_t mins = (total_secs % 3600) / 60;
    uint32_t secs = total_secs % 60;

    int written;
    if (days > 0)
    {
        written = snprintf(buf, len, "%lud %luh %lum %lus", (unsigned long)days, (unsigned long)hours,
                           (unsigned long)mins, (unsigned long)secs);
    }
    else if (hours > 0)
    {
        written = snprintf(buf, len, "%luh %lum %lus", (unsigned long)hours, (unsigned long)mins, (unsigned long)secs);
    }
    else if (mins > 0)
    {
        written = snprintf(buf, len, "%lum %lus", (unsigned long)mins, (unsigned long)secs);
    }
    else
    {
        written = snprintf(buf, len, "%lus", (unsigned long)secs);
    }

    if (written < 0 || (size_t)written >= len)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

void system_restart(void)
{
    ESP_LOGI(TAG, "Restarting...");
    esp_restart();
}
