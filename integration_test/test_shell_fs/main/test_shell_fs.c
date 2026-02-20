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

static void seed_test_files(void)
{
    flash_format();
    vfs_write_file("/flash/hello.txt", "Hello, COS!\n", 12);
    vfs_write_file("/flash/binary.bin", "\x00\x01\x02\x41\x42\x43", 6);
}

void app_main(void)
{
    init_nvs();
    ESP_ERROR_CHECK(filesystem_init());
    seed_test_files();
    ESP_ERROR_CHECK(shell_init());
    ESP_LOGI(TAG, "SHELL_READY");
}
