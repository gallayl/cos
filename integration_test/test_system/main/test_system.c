#include "filesystem.h"
#include "shell.h"
#include "system.h"

#include "esp_log.h"
#include "nvs_flash.h"

static const char *const TAG = "test_system";

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

static const char UPTIME_SCRIPT[] = "var __cos_mod_1 = cos_import(\"@cos/system\")\n"
                                    "var system = __cos_mod_1.find(\"default\")\n"
                                    "class Uptime\n"
                                    "    def run(_args)\n"
                                    "        print(\"Up \" + system.uptime())\n"
                                    "        return 0\n"
                                    "    end\n"
                                    "end\n"
                                    "\n"
                                    "var __cos_entry = Uptime()\n";

static void seed_test_files(void)
{
    /* Seed both CWD-resolved and stock script locations for `uptime`. */
    ESP_ERROR_CHECK(vfs_write_file("/flash/uptime.be", UPTIME_SCRIPT, sizeof(UPTIME_SCRIPT) - 1));
    ESP_ERROR_CHECK(vfs_mkdir("/flash/scripts"));
    ESP_ERROR_CHECK(vfs_write_file("/flash/scripts/uptime.be", UPTIME_SCRIPT, sizeof(UPTIME_SCRIPT) - 1));
}

void app_main(void)
{
    init_nvs();
    ESP_ERROR_CHECK(flash_format());
    ESP_ERROR_CHECK(filesystem_init());
    seed_test_files();
    ESP_ERROR_CHECK(system_init());
    ESP_ERROR_CHECK(shell_init());
    ESP_LOGI(TAG, "SYSTEM_READY");
}
