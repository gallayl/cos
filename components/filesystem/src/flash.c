#include "flash.h"
#include "filesystem.h"

#include "esp_littlefs.h"
#include "esp_log.h"

static const char *const TAG = "flash";

esp_err_t flash_init(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = FLASH_MOUNT_POINT,
        .partition_label = FLASH_PARTITION_LABEL,
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0;
    size_t used = 0;
    esp_littlefs_info(FLASH_PARTITION_LABEL, &total, &used);
    ESP_LOGI(TAG, "LittleFS mounted: total=%u, used=%u", (unsigned)total, (unsigned)used);

    return ESP_OK;
}

void flash_deinit(void)
{
    esp_vfs_littlefs_unregister(FLASH_PARTITION_LABEL);
}

esp_err_t flash_format(void)
{
    ESP_LOGW(TAG, "Formatting LittleFS...");
    flash_deinit();

    esp_err_t ret = esp_littlefs_format(FLASH_PARTITION_LABEL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Format failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = flash_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Re-mount after format failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

size_t flash_get_total_bytes(void)
{
    size_t total = 0;
    size_t used = 0;
    if (esp_littlefs_info(FLASH_PARTITION_LABEL, &total, &used) == ESP_OK)
    {
        return total;
    }
    return 0;
}

size_t flash_get_used_bytes(void)
{
    size_t total = 0;
    size_t used = 0;
    if (esp_littlefs_info(FLASH_PARTITION_LABEL, &total, &used) == ESP_OK)
    {
        return used;
    }
    return 0;
}
