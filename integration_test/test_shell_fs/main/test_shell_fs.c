#include "filesystem.h"
#include "shell.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include <string.h>

static const char *const TAG = "test_shell_fs";
static char s_large_file[1200];
static const char s_berry_api_script[] = "var listing = fs.list(\"/flash\")\n"
                                         "var empty = fs.list(\"/flash/berry-empty\")\n"
                                         "var invalid = fs.list(\"/flash/../\")\n"
                                         "var no_network = net.ping(\"example.com\")\n"
                                         "var short_timeout = net.ping(\"example.com\", 99)\n"
                                         "var invalid_host = net.ping(\"bad host\")\n"
                                         "if listing[\"ok\"] && listing[\"count\"] > 0 && listing[\"entries\"][0][\"name\"] != nil\n"
                                         "    log(\"FS_LIST_OK\")\n"
                                         "end\n"
                                         "if empty[\"ok\"] && empty[\"count\"] == 0\n"
                                         "    log(\"FS_EMPTY_OK\")\n"
                                         "end\n"
                                         "if !invalid[\"ok\"] && invalid[\"error\"] == \"ESP_ERR_INVALID_ARG\"\n"
                                         "    log(\"FS_INVALID_PATH_OK\")\n"
                                         "end\n"
                                         "if !no_network[\"ok\"] && no_network[\"error\"] == \"ESP_ERR_INVALID_STATE\"\n"
                                         "    log(\"NET_NO_WIFI_OK\")\n"
                                         "end\n"
                                         "if !short_timeout[\"ok\"] && short_timeout[\"error\"] == \"ESP_ERR_INVALID_ARG\"\n"
                                         "    log(\"NET_TIMEOUT_BOUNDS_OK\")\n"
                                         "end\n"
                                         "if !invalid_host[\"ok\"] && invalid_host[\"error\"] == \"ESP_ERR_INVALID_ARG\"\n"
                                         "    log(\"NET_INVALID_HOST_OK\")\n"
                                         "end\n";
static const char s_berry_input_script[] = "class InputApp\n"
                                           "    def setup(content, width, height)\n"
                                           "        console.read_line(\"Input: \", def (value)\n"
                                           "            if value == nil\n"
                                           "                log(\"INPUT_CANCELLED\")\n"
                                           "            else\n"
                                           "                log(\"INPUT_VALUE:\" + value)\n"
                                           "            end\n"
                                           "            ui.exit()\n"
                                           "        end)\n"
                                           "    end\n"
                                           "end\n";

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
    ESP_ERROR_CHECK(flash_format());
    ESP_ERROR_CHECK(vfs_write_file("/flash/hello.txt", "Hello, COS!\n", 12));
    ESP_ERROR_CHECK(vfs_write_file("/flash/binary.bin", "\x00\x01\x02\x41\x42\x43", 6));
    ESP_ERROR_CHECK(vfs_mkdir("/flash/berry-empty"));
    ESP_ERROR_CHECK(vfs_write_file("/flash/berry_api.be", s_berry_api_script, sizeof(s_berry_api_script) - 1));
    ESP_ERROR_CHECK(vfs_write_file("/flash/berry_input.be", s_berry_input_script, sizeof(s_berry_input_script) - 1));

    memset(s_large_file, 'A', 600);
    memset(s_large_file + 600, 'B', sizeof(s_large_file) - 600);
    ESP_ERROR_CHECK(vfs_write_file("/flash/large.txt", s_large_file, sizeof(s_large_file)));
}

void app_main(void)
{
    init_nvs();
    ESP_ERROR_CHECK(filesystem_init());
    seed_test_files();
    ESP_ERROR_CHECK(shell_init());
    ESP_LOGI(TAG, "SHELL_READY");
}
