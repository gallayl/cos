/**
 * @file littlefs_api.c
 * @brief Maps the HAL of esp_partition <-> littlefs
 * @author Brian Pugh
 */

// #define ESP_LOCAL_LOG_LEVEL ESP_LOG_INFO

#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "littlefs/lfs.h"
#include "littlefs_api.h"

#ifdef CONFIG_LITTLEFS_WDT_RESET
#include "esp_task_wdt.h"
#endif

#ifdef CONFIG_LITTLEFS_MMAP_PARTITION
int littlefs_esp_part_read_mmap(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer,
                                lfs_size_t size)
{
    esp_littlefs_t *efs = c->context;
    if (!efs || !efs->partition || !efs->mmap_data || !buffer)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "mmap read invalid context/buffer");
        return LFS_ERR_INVAL;
    }
    size_t part_off = (block * c->block_size) + off;
    if (part_off > efs->partition->size || size > efs->partition->size - part_off)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "attempt to read out of bounds of mmaped region 0x%08x-0x%08x (part size 0x%08x)",
                 (unsigned int)part_off, (unsigned int)(part_off + size), (unsigned int)efs->partition->size);
        return LFS_ERR_IO;
    }
    memcpy(buffer, (const uint8_t *)efs->mmap_data + part_off, size);
    return 0;
}
#endif

int littlefs_esp_part_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    esp_littlefs_t *efs = c->context;
    if (!efs || !efs->partition || !buffer)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "part_read invalid context/buffer");
        return LFS_ERR_INVAL;
    }
    size_t part_off = (block * c->block_size) + off;
    if (part_off > efs->partition->size || size > efs->partition->size - part_off)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "part_read out of bounds addr 0x%08x size 0x%08x (part size 0x%08x)",
                 (unsigned int)part_off, (unsigned int)size, (unsigned int)efs->partition->size);
        return LFS_ERR_IO;
    }

#ifdef CONFIG_LITTLEFS_WDT_RESET
    esp_task_wdt_reset();
#endif

    ESP_LOGV(ESP_LITTLEFS_TAG, "part_read start blk=%u off=%u size=%u", (unsigned)block, (unsigned)off, (unsigned)size);
    esp_err_t err = esp_partition_read(efs->partition, part_off, buffer, size);
    ESP_LOGV(ESP_LITTLEFS_TAG, "part_read done blk=%u err=%d", (unsigned)block, (int)err);
    if (err)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "failed to read addr 0x%08x, size 0x%08x, err %d (%s)", (unsigned int)part_off,
                 (unsigned int)size, err, esp_err_to_name(err));
        return LFS_ERR_IO;
    }
    return 0;
}

int littlefs_esp_part_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer,
                            lfs_size_t size)
{
    esp_littlefs_t *efs = c->context;
    if (!efs || !efs->partition || !buffer)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "part_write invalid context/buffer");
        return LFS_ERR_INVAL;
    }
    size_t part_off = (block * c->block_size) + off;
    if (part_off > efs->partition->size || size > efs->partition->size - part_off)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "part_write out of bounds addr 0x%08x size 0x%08x (part size 0x%08x)",
                 (unsigned int)part_off, (unsigned int)size, (unsigned int)efs->partition->size);
        return LFS_ERR_IO;
    }

#ifdef CONFIG_LITTLEFS_WDT_RESET
    esp_task_wdt_reset();
#endif

    ESP_LOGV(ESP_LITTLEFS_TAG, "part_write start blk=%u off=%u size=%u", (unsigned)block, (unsigned)off,
             (unsigned)size);
    esp_err_t err = esp_partition_write(efs->partition, part_off, buffer, size);
    ESP_LOGV(ESP_LITTLEFS_TAG, "part_write done blk=%u err=%d", (unsigned)block, (int)err);
    if (err)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "failed to write addr 0x%08x, size 0x%08x, err %d (%s)", (unsigned int)part_off,
                 (unsigned int)size, err, esp_err_to_name(err));
        return LFS_ERR_IO;
    }
    return 0;
}

int littlefs_esp_part_erase(const struct lfs_config *c, lfs_block_t block)
{
    esp_littlefs_t *efs = c->context;
    if (!efs || !efs->partition)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "part_erase invalid context");
        return LFS_ERR_INVAL;
    }
    size_t part_off = block * c->block_size;
    if (part_off > efs->partition->size || c->block_size > efs->partition->size - part_off)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "part_erase out of bounds addr 0x%08x size 0x%08x (part size 0x%08x)",
                 (unsigned int)part_off, (unsigned int)c->block_size, (unsigned int)efs->partition->size);
        return LFS_ERR_IO;
    }

#ifdef CONFIG_LITTLEFS_WDT_RESET
    esp_task_wdt_reset();
#endif

    ESP_LOGV(ESP_LITTLEFS_TAG, "part_erase start blk=%u", (unsigned)block);
    esp_err_t err = esp_partition_erase_range(efs->partition, part_off, c->block_size);
    ESP_LOGV(ESP_LITTLEFS_TAG, "part_erase done blk=%u err=%d", (unsigned)block, (int)err);
    if (err)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "failed to erase addr 0x%08x, size 0x%08x, err %d (%s)", (unsigned int)part_off,
                 (unsigned int)c->block_size, err, esp_err_to_name(err));
        return LFS_ERR_IO;
    }
    return 0;
}

int littlefs_esp_part_sync(const struct lfs_config *c)
{
    /* Unnecessary for esp-idf */
    return 0;
}
