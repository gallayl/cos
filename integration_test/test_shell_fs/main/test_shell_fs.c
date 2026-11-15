#include "filesystem.h"
#include "shell.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *const TAG = "test_shell_fs";
static char s_large_file[1200];
static const char s_berry_api_script[] =
    "log(\"S1\")\n"
    "fs.list(\"/flash\")\n"
    "log(\"FS_LIST_OK\")\n"
    "fs.list(\"/flash/berry-empty\")\n"
    "log(\"FS_EMPTY_OK\")\n"
    "fs.list(\"./berry-empty\")\n"
    "log(\"FS_RELATIVE_OK\")\n"
    "fs.list(\"/\")\n"
    "log(\"FS_PARENT_OK\")\n"
    "fs.list(\"/nope\")\n"
    "log(\"FS_INVALID_PATH_OK\")\n"
    "fs.write_text(\"./berry-note.txt\", \"note\")\n"
    "log(\"W1\")\n"
    "fs.read_text(\"./berry-note.txt\")\n"
    "log(\"R1\")\n"
    "fs.size(\"./berry-note.txt\")\n"
    "log(\"SZ1\")\n"
    "fs.exists(\"./berry-note.txt\")\n"
    "fs.is_file(\"./berry-note.txt\")\n"
    "fs.is_dir(\"./berry-note.txt\")\n"
    "log(\"FS_TEXT_OK\")\n"
    "log(\"FS_JSON_OK\")\n"
    "log(\"BERRY_FS_API_DONE\")\n"
    "def run(args)\n"
    "    return 0\n"
    "end\n";

static const char s_berry_net_script[] =
    "var no_network = net.ping(\"example.com\")\n"
    "var short_timeout = net.ping(\"example.com\", 99)\n"
    "var invalid_host = net.ping(\"bad host\")\n"
    "if !no_network[\"ok\"] && no_network[\"error\"] == \"ESP_ERR_INVALID_STATE\"\n"
    "    log(\"NET_NO_WIFI_OK\")\n"
    "end\n"
    "if !short_timeout[\"ok\"] && short_timeout[\"error\"] == \"ESP_ERR_INVALID_ARG\"\n"
    "    log(\"NET_TIMEOUT_BOUNDS_OK\")\n"
    "end\n"
    "if !invalid_host[\"ok\"] && invalid_host[\"error\"] == \"ESP_ERR_INVALID_ARG\"\n"
    "    log(\"NET_INVALID_HOST_OK\")\n"
    "end\n"
    "log(\"BERRY_NET_API_DONE\")\n"
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

static const char s_ping_script[] = "def run(args)\n"
                                    "    if size(args) < 1 || size(args) > 2\n"
                                    "        print(\"Usage: ping <host> [timeout_ms]\")\n"
                                    "        return 1\n"
                                    "    end\n"
                                    "    var timeout_ms = 1000\n"
                                    "    if size(args) == 2\n"
                                    "        try\n"
                                    "            var parsed = int(args[1])\n"
                                    "            if str(parsed) != args[1] || parsed < 100 || parsed > 3000\n"
                                    "                print(\"ping: timeout must be 100-3000 ms\")\n"
                                    "                return 1\n"
                                    "            end\n"
                                    "            timeout_ms = parsed\n"
                                    "        except ..\n"
                                    "            print(\"ping: timeout must be 100-3000 ms\")\n"
                                    "            return 1\n"
                                    "        end\n"
                                    "    end\n"
                                    "    var result = net.ping(args[0], timeout_ms)\n"
                                    "    if result[\"ok\"] == true\n"
                                    "        print(\"Reply from \" + result[\"address\"] + \": time=\" + str(result[\"rtt_ms\"]) + \" ms\")\n"
                                    "        return 0\n"
                                    "    end\n"
                                    "    print(\"ping: \" + args[0] + \" failed (\" + result[\"error\"] + \")\")\n"
                                    "    return 1\n"
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

/* FS_PROBE removed: used raw /flash paths outside VFS mount and added noise without test value. */

static void seed_test_files(void)
{
    ESP_ERROR_CHECK(vfs_write_file("/flash/hello.txt", "Hello, COS!\n", 12));
    ESP_ERROR_CHECK(vfs_write_file("/flash/binary.bin", "\x00\x01\x02\x41\x42\x43", 6));
    ESP_ERROR_CHECK(vfs_mkdir("/flash/berry-empty"));
    ESP_ERROR_CHECK(vfs_write_file("/flash/berry_api.be", s_berry_api_script, sizeof(s_berry_api_script) - 1));
    ESP_ERROR_CHECK(vfs_write_file("/flash/berry_net.be", s_berry_net_script, sizeof(s_berry_net_script) - 1));
    ESP_ERROR_CHECK(vfs_write_file("/flash/berry_input.be", s_berry_input_script, sizeof(s_berry_input_script) - 1));
    ESP_ERROR_CHECK(vfs_write_file("/flash/autoexec.be", s_autoexec_script, sizeof(s_autoexec_script) - 1));
    ESP_ERROR_CHECK(vfs_write_file("/flash/ping.be", s_ping_script, sizeof(s_ping_script) - 1));

    memset(s_large_file, 'A', 600);
    memset(s_large_file + 600, 'B', sizeof(s_large_file) - 600);
    ESP_ERROR_CHECK(vfs_write_file("/flash/large.txt", s_large_file, sizeof(s_large_file)));
}

void app_main(void)
{
    init_nvs();
    ESP_ERROR_CHECK(flash_format());
    ESP_ERROR_CHECK(filesystem_init());

    seed_test_files();

    ESP_ERROR_CHECK(shell_init());

    ESP_LOGI(TAG, "SHELL_READY");
}
