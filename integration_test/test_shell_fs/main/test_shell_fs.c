#include "filesystem.h"
#include "shell.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include <string.h>

static const char *const TAG = "test_shell_fs";
static char s_large_file[1200];
static const char s_berry_api_script[] =
    "var listing = fs.list(\"/flash\")\n"
    "var empty = fs.list(\"/flash/berry-empty\")\n"
    "var relative = fs.list(\"./\")\n"
    "var parent = fs.list(\"./../\")\n"
    "var invalid = fs.list(\"/nope\")\n"
    "var written = fs.write_text(\"./berry-note.txt\", \"note\")\n"
    "var saved = fs.write_json(\"./berry-note.json\", "
    "{\"label\": \"cos\", \"flags\": [true], \"child\": {\"ok\": true}})\n"
    "var no_network = net.ping(\"example.com\")\n"
    "var short_timeout = net.ping(\"example.com\", 99)\n"
    "var invalid_host = net.ping(\"bad host\")\n"
    "if listing[\"ok\"]\n"
    "    log(\"FS_LIST_STEP_OK\")\n"
    "end\n"
    "if listing[\"ok\"] && listing[\"count\"] > 0 && "
    "listing[\"entries\"][0][\"name\"] != nil\n"
    "    log(\"FS_LIST_OK\")\n"
    "end\n"
    "if empty[\"ok\"] && empty[\"count\"] == 0\n"
    "    log(\"FS_EMPTY_OK\")\n"
    "end\n"
    "if relative[\"ok\"] && relative[\"count\"] > 0\n"
    "    log(\"FS_RELATIVE_OK\")\n"
    "end\n"
    "if parent[\"ok\"] && parent[\"count\"] > 0\n"
    "    log(\"FS_PARENT_OK\")\n"
    "end\n"
    "if !invalid[\"ok\"] && invalid[\"error\"] == \"ESP_ERR_INVALID_ARG\"\n"
    "    log(\"FS_INVALID_PATH_OK\")\n"
    "end\n"
    "if written == true\n"
    "    var note = fs.read_text(\"./berry-note.txt\")\n"
    "    var bytes = fs.size(\"./berry-note.txt\")\n"
    "    if note[\"ok\"] && note[\"text\"] == \"note\" && bytes[\"ok\"] && bytes[\"size\"] == 4 && "
    "fs.exists(\"./berry-note.txt\") && fs.is_file(\"./berry-note.txt\") && "
    "!fs.is_dir(\"./berry-note.txt\")\n"
    "        log(\"FS_TEXT_OK\")\n"
    "    end\n"
    "end\n"
    "if saved == true\n"
    "    var parsed = fs.read_json(\"./berry-note.json\")\n"
    "    if parsed[\"ok\"] && parsed[\"value\"][\"label\"] == \"cos\" && "
    "parsed[\"value\"][\"flags\"][0] == true && parsed[\"value\"][\"child\"][\"ok\"] == true\n"
    "        log(\"FS_JSON_OK\")\n"
    "    end\n"
    "end\n"
    "if !no_network[\"ok\"] && "
    "no_network[\"error\"] == \"ESP_ERR_INVALID_STATE\"\n"
    "    log(\"NET_NO_WIFI_OK\")\n"
    "end\n"
    "if !short_timeout[\"ok\"] && "
    "short_timeout[\"error\"] == \"ESP_ERR_INVALID_ARG\"\n"
    "    log(\"NET_TIMEOUT_BOUNDS_OK\")\n"
    "end\n"
    "if !invalid_host[\"ok\"] && "
    "invalid_host[\"error\"] == \"ESP_ERR_INVALID_ARG\"\n"
    "    log(\"NET_INVALID_HOST_OK\")\n"
    "end\n"
    "log(\"BERRY_API_DONE\")\n"
    "def run(args)\n"
    "    return 0\n"
    "end\n";
static const char s_berry_input_script[] = "var started = console.read_line(\"Input: \", def (value)\n"
                                           "    if value == nil\n"
                                           "        log(\"INPUT_CANCELLED\")\n"
                                           "    else\n"
                                           "        log(\"INPUT_VALUE:\" + str(value))\n"
                                           "    end\n"
                                           "end)\n"
                                           "if started == true\n"
                                           "    log(\"INPUT_STARTED\")\n"
                                           "else\n"
                                           "    log(\"INPUT_START_ERROR:\" + str(started))\n"
                                           "end\n"
                                           "def run(args)\n"
                                           "    return 0\n"
                                           "end\n";
static const char s_autoexec_script[] = "log(\"AUTOEXEC_OK\")\n"
                                        "def run(args)\n"
                                        "    return 0\n"
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
    ESP_ERROR_CHECK(vfs_write_file("/flash/autoexec.be", s_autoexec_script, sizeof(s_autoexec_script) - 1));

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
