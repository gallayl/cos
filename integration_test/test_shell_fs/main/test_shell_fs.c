#include "filesystem.h"
#include "shell.h"

#include "esp_log.h"
#include "nvs_flash.h"

static const char *const TAG = "test_shell_fs";

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void app_main(void)
{
    init_nvs();
    ESP_ERROR_CHECK(filesystem_init());
    ESP_ERROR_CHECK(shell_init());
    ESP_LOGI(TAG, "SHELL_READY");
}
