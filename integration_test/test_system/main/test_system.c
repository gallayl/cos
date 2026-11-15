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

static const char MEMORY_SCRIPT[] =
    "var __cos_mod_1 = cos_import(\"@cos/system\")\n"
    "var system = __cos_mod_1.find(\"default\")\n"
    "class Memory\n"
    "    def run(args)\n"
    "        if size(args) > 0\n"
    "            print(\"Usage: memory\")\n"
    "            return 1\n"
    "        end\n"
    "        var info = system.meminfo()\n"
    "        if info[\"ok\"] != true\n"
    "            print(\"memory: failed (\" + info[\"error\"] + \")\")\n"
    "            return 1\n"
    "        end\n"
    "        print(\"Free heap:      \" + str(info[\"free_heap\"]) + \" bytes\")\n"
    "        print(\"Min free heap:  \" + str(info[\"min_free_heap\"]) + \" bytes\")\n"
    "        print(\"Total heap:     \" + str(info[\"total_heap\"]) + \" bytes\")\n"
    "        print(\"Max alloc:      \" + str(info[\"max_alloc_block\"]) + \" bytes\")\n"
    "        print(\"Internal free:  \" + str(info[\"internal_free_heap\"]) + \" bytes\")\n"
    "        print(\"Internal min:   \" + str(info[\"internal_min_free_heap\"]) + \" bytes\")\n"
    "        print(\"Internal total: \" + str(info[\"internal_total_heap\"]) + \" bytes\")\n"
    "        print(\"Internal max:   \" + str(info[\"internal_max_alloc_block\"]) + \" bytes\")\n"
    "        return 0\n"
    "    end\n"
    "end\n"
    "\n"
    "var __cos_entry = Memory()\n";

static void seed_test_files(void)
{
    /* Seed CWD-resolved and stock script locations for script-backed commands. */
    ESP_ERROR_CHECK(vfs_write_file("/flash/uptime.be", UPTIME_SCRIPT, sizeof(UPTIME_SCRIPT) - 1));
    ESP_ERROR_CHECK(vfs_write_file("/flash/memory.be", MEMORY_SCRIPT, sizeof(MEMORY_SCRIPT) - 1));
    ESP_ERROR_CHECK(vfs_write_file("/flash/mem.be", MEMORY_SCRIPT, sizeof(MEMORY_SCRIPT) - 1));
    ESP_ERROR_CHECK(vfs_mkdir("/flash/scripts"));
    ESP_ERROR_CHECK(vfs_write_file("/flash/scripts/uptime.be", UPTIME_SCRIPT, sizeof(UPTIME_SCRIPT) - 1));
    ESP_ERROR_CHECK(vfs_write_file("/flash/scripts/memory.be", MEMORY_SCRIPT, sizeof(MEMORY_SCRIPT) - 1));
    ESP_ERROR_CHECK(vfs_write_file("/flash/scripts/mem.be", MEMORY_SCRIPT, sizeof(MEMORY_SCRIPT) - 1));
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
