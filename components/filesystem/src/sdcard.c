#include "sdcard.h"
#include "cyd_sdcard_pins.h"
#include "filesystem.h"

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static const char *const TAG = "sdcard";

static sdmmc_card_t *s_card = NULL;
static bool s_spi_initialized = false;
static bool s_mounted = false;

static esp_err_t ensure_spi_bus(void)
{
    if (s_spi_initialized)
    {
        return ESP_OK;
    }

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SDCARD_PIN_MOSI,
        .miso_io_num = SDCARD_PIN_MISO,
        .sclk_io_num = SDCARD_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t err = spi_bus_initialize(SDCARD_SPI_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (err == ESP_ERR_INVALID_STATE)
    {
        /* Bus already initialized (e.g. by display driver) -- that's fine */
        s_spi_initialized = true;
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(err));
        return err;
    }

    s_spi_initialized = true;
    return ESP_OK;
}

esp_err_t sdcard_init(void)
{
    return sdcard_mount();
}

esp_err_t sdcard_mount(void)
{
    if (s_mounted)
    {
        ESP_LOGW(TAG, "SD card already mounted");
        return ESP_OK;
    }

    esp_err_t err = ensure_spi_bus();
    if (err != ESP_OK)
    {
        return err;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = (size_t)(16 * 1024),
    };

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = (gpio_num_t)SDCARD_PIN_CS;
    slot_cfg.host_id = SDCARD_SPI_HOST;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SDCARD_SPI_HOST;

    err = esp_vfs_fat_sdspi_mount(SDCARD_MOUNT_POINT, &host, &slot_cfg, &mount_cfg, &s_card);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "SD card mount failed: %s", esp_err_to_name(err));
        return err;
    }

    s_mounted = true;
    ESP_LOGI(TAG, "SD card mounted (%s)", sdcard_get_type_name());
    return ESP_OK;
}

esp_err_t sdcard_unmount(void)
{
    if (!s_mounted)
    {
        ESP_LOGW(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    esp_vfs_fat_sdcard_unmount(SDCARD_MOUNT_POINT, s_card);
    s_card = NULL;
    s_mounted = false;
    ESP_LOGI(TAG, "SD card unmounted");
    return ESP_OK;
}

bool sdcard_is_mounted(void)
{
    return s_mounted;
}

const char *sdcard_get_type_name(void)
{
    if (!s_mounted || s_card == NULL)
    {
        return "NONE";
    }
    if (s_card->is_mmc)
    {
        return "MMC";
    }
    if (s_card->ocr & (1 << 30))
    {
        return "SDHC";
    }
    return "SD";
}

uint64_t sdcard_get_total_bytes(void)
{
    if (!s_mounted)
    {
        return 0;
    }

    uint64_t total = 0;
    uint64_t free_bytes = 0;
    if (esp_vfs_fat_info(SDCARD_MOUNT_POINT, &total, &free_bytes) == ESP_OK)
    {
        return total;
    }
    return 0;
}

uint64_t sdcard_get_used_bytes(void)
{
    if (!s_mounted)
    {
        return 0;
    }

    uint64_t total = 0;
    uint64_t free_bytes = 0;
    if (esp_vfs_fat_info(SDCARD_MOUNT_POINT, &total, &free_bytes) == ESP_OK)
    {
        return total - free_bytes;
    }
    return 0;
}
