#include "calibration.h"
#include "display.h"
#include "filesystem.h"
#include "shell.h"

#include "esp_log.h"
#include "nvs_flash.h"

static const char *const TAG = "main";

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS partition corrupted, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

void app_main(void)
{
    ESP_LOGI(TAG, "COS starting up...");

    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(display_init());
    ESP_ERROR_CHECK(calibration_init());
    ESP_ERROR_CHECK(filesystem_init());
    ESP_ERROR_CHECK(shell_init());

    ESP_LOGI(TAG, "COS ready");
}
