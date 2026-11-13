/**
 * @file littlefs_sdmmc.c
 * @brief Maps the HAL of sdmmc driver <-> littlefs
 * @author Jackson Ming Hu <jackson@smartguide.com.au>
 */

#include "littlefs_api.h"
#include <sdmmc_cmd.h>
#include <sys/param.h>

#if CONFIG_LITTLEFS_SDMMC_SUPPORT

int littlefs_sdmmc_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    esp_littlefs_t *efs = c->context;
    uint32_t part_off = (block * c->block_size) + off;
    size_t sector_size = efs->sdcard->csd.sector_size;
    if (sector_size == 0 || (part_off % sector_size) != 0 || (size % sector_size) != 0)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "Invalid SDMMC read: off 0x%08lx, block 0x%08lx, size %lu, sector %u", part_off, off,
                 block, size, (unsigned)sector_size);
        return LFS_ERR_IO;
    }

    uint32_t start_block = part_off / sector_size;
    size_t block_count = size / sector_size;
    esp_err_t ret = sdmmc_read_sectors(efs->sdcard, buffer, start_block, block_count);
    if (ret != ESP_OK)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "Failed to read addr 0x%08lx: off 0x%08lx, block 0x%08lx, size %lu, err=0x%x",
                 part_off, off, block, size, ret);
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

int littlefs_sdmmc_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer,
                         lfs_size_t size)
{
    esp_littlefs_t *efs = c->context;
    uint32_t part_off = (block * c->block_size) + off;
    size_t sector_size = efs->sdcard->csd.sector_size;
    if (sector_size == 0 || (part_off % sector_size) != 0 || (size % sector_size) != 0)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "Invalid SDMMC write: off 0x%08lx, block 0x%08lx, size %lu, sector %u", part_off,
                 off, block, size, (unsigned)sector_size);
        return LFS_ERR_IO;
    }

    uint32_t start_block = part_off / sector_size;
    size_t block_count = size / sector_size;
    esp_err_t ret = sdmmc_write_sectors(efs->sdcard, buffer, start_block, block_count);
    if (ret != ESP_OK)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "Failed to write addr 0x%08lx: off 0x%08lx, block 0x%08lx, size %lu, err=0x%x",
                 part_off, off, block, size, ret);
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

int littlefs_sdmmc_erase(const struct lfs_config *c, lfs_block_t block)
{
    esp_littlefs_t *efs = c->context;
    esp_err_t ret = sdmmc_erase_sectors(efs->sdcard, block, 1, SDMMC_ERASE_ARG);
    if (ret != ESP_OK)
    {
        ESP_LOGE(ESP_LITTLEFS_TAG, "Failed to erase block %lu: ret=0x%x %s", block, ret, esp_err_to_name(ret));
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

int littlefs_sdmmc_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK; // Doesn't require & doesn't support sync
}
#endif
