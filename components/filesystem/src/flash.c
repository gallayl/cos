#include "flash.h"
#include "filesystem.h"

#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_partition.h"

#include <inttypes.h>

static const char *const TAG = "flash";

static void flash_log_partition_info(void)
{
    const esp_partition_t *part =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, FLASH_PARTITION_LABEL);
    if (part != NULL)
    {
        ESP_LOGI(TAG, "Partition '%s': addr=0x%08" PRIx32 " size=0x%08" PRIx32 " type=%u subtype=0x%02x", part->label,
                 part->address, part->size, part->type, part->subtype);
    }
    else
    {
        ESP_LOGE(TAG, "Partition '%s' not found", FLASH_PARTITION_LABEL);
    }
}

esp_err_t flash_init(void)
{
    flash_log_partition_info();
    if (esp_littlefs_mounted(FLASH_PARTITION_LABEL))
    {
        ESP_LOGD(TAG, "LittleFS already mounted");
        return ESP_OK;
    }

    esp_vfs_littlefs_conf_t conf = {
        .base_path = FLASH_MOUNT_POINT,
        .partition_label = FLASH_PARTITION_LABEL,
        .format_if_mount_failed = 0,
        .dont_mount = 0,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "LittleFS mount failed (%s) -- run 'format' command to initialize", esp_err_to_name(ret));
        return ESP_OK;
    }

    size_t total = 0;
    size_t used = 0;
    if (esp_littlefs_info(FLASH_PARTITION_LABEL, &total, &used) == ESP_OK)
    {
        ESP_LOGI(TAG, "LittleFS mounted: total=%u, used=%u", (unsigned)total, (unsigned)used);
    }
    else
    {
        ESP_LOGI(TAG, "LittleFS mounted");
    }

    return ESP_OK;
}

void flash_deinit(void)
{
    if (esp_littlefs_mounted(FLASH_PARTITION_LABEL))
    {
        esp_vfs_littlefs_unregister(FLASH_PARTITION_LABEL);
    }
}

esp_err_t flash_format(void)
{
    ESP_LOGW(TAG, "Formatting LittleFS...");
    flash_log_partition_info();
    flash_deinit();

    esp_err_t ret = esp_littlefs_format(FLASH_PARTITION_LABEL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Format failed: %s", esp_err_to_name(ret));
        return ret;
    }

    (void)flash_init();
    if (!esp_littlefs_mounted(FLASH_PARTITION_LABEL))
    {
        ESP_LOGE(TAG, "Re-mount after format failed; filesystem not available");
        return ESP_FAIL;
    }

    return ESP_OK;
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
