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

static void run_fs_probe(const char *phase)
{
    const char *stdio_path = "/flash/probe_stdio.txt";
    const char *posix_path = "/flash/probe_posix.txt";
    const char *dir_path = "/flash/probe_dir";
    const char *content = "LittleFS test";
    char read_buf[32] = {0};
    struct stat st;

    ESP_LOGI(TAG, "FS_PROBE[%s]: begin", phase);

    /* stdio write/create */
    ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fopen(w) before (%s)", phase, stdio_path);
    FILE *file = fopen(stdio_path, "w");
    ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fopen(w) after (%p)", phase, (void *)file);
    if (file == NULL)
    {
        ESP_LOGE(TAG, "FS_PROBE[%s]: stdio fopen(w) failed errno=%d", phase, errno);
    }
    else
    {
        ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fwrite before", phase);
        size_t written = fwrite(content, 1, strlen(content), file);
        ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fwrite after (%u)", phase, (unsigned)written);
        ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fclose before", phase);
        int close_rc = fclose(file);
        ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fclose after (%d)", phase, close_rc);
    }

    /* stdio read */
    ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fopen(r) before (%s)", phase, stdio_path);
    file = fopen(stdio_path, "r");
    ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fopen(r) after (%p)", phase, (void *)file);
    if (file == NULL)
    {
        ESP_LOGE(TAG, "FS_PROBE[%s]: stdio fopen(r) failed errno=%d", phase, errno);
    }
    else
    {
        ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fread before", phase);
        size_t bytes_read = fread(read_buf, 1, sizeof(read_buf) - 1, file);
        ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fread after (%u)", phase, (unsigned)bytes_read);
        ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fclose(r) before", phase);
        int close_rc = fclose(file);
        ESP_LOGI(TAG, "FS_PROBE[%s]: stdio fclose(r) after (%d)", phase, close_rc);
    }

    /* POSIX open/write/close */
    ESP_LOGI(TAG, "FS_PROBE[%s]: posix open(w) before (%s)", phase, posix_path);
    int fd = open(posix_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    ESP_LOGI(TAG, "FS_PROBE[%s]: posix open(w) after (%d)", phase, fd);
    if (fd < 0)
    {
        ESP_LOGE(TAG, "FS_PROBE[%s]: posix open(w) failed errno=%d", phase, errno);
    }
    else
    {
        ESP_LOGI(TAG, "FS_PROBE[%s]: posix write before", phase);
        ssize_t written = write(fd, content, strlen(content));
        ESP_LOGI(TAG, "FS_PROBE[%s]: posix write after (%d)", phase, (int)written);
        ESP_LOGI(TAG, "FS_PROBE[%s]: posix close before", phase);
        int close_rc = close(fd);
        ESP_LOGI(TAG, "FS_PROBE[%s]: posix close after (%d)", phase, close_rc);
    }

    /* stat */
    ESP_LOGI(TAG, "FS_PROBE[%s]: stat before (%s)", phase, stdio_path);
    int stat_rc = stat(stdio_path, &st);
    ESP_LOGI(TAG, "FS_PROBE[%s]: stat after (%d, size=%ld)", phase, stat_rc, stat_rc == 0 ? (long)st.st_size : -1L);

    /* mkdir */
    ESP_LOGI(TAG, "FS_PROBE[%s]: mkdir before (%s)", phase, dir_path);
    int mkdir_rc = mkdir(dir_path, 0775);
    ESP_LOGI(TAG, "FS_PROBE[%s]: mkdir after (%d, errno=%d)", phase, mkdir_rc, mkdir_rc == 0 ? 0 : errno);

    /* unlink */
    ESP_LOGI(TAG, "FS_PROBE[%s]: unlink before (%s)", phase, posix_path);
    int unlink_rc = unlink(posix_path);
    ESP_LOGI(TAG, "FS_PROBE[%s]: unlink after (%d, errno=%d)", phase, unlink_rc, unlink_rc == 0 ? 0 : errno);

    /* cleanup probe artifacts (best-effort) */
    ESP_LOGI(TAG, "FS_PROBE[%s]: cleanup unlink stdio before", phase);
    (void)unlink(stdio_path);
    ESP_LOGI(TAG, "FS_PROBE[%s]: cleanup unlink stdio after", phase);
    ESP_LOGI(TAG, "FS_PROBE[%s]: cleanup rmdir before", phase);
    (void)rmdir(dir_path);
    ESP_LOGI(TAG, "FS_PROBE[%s]: cleanup rmdir after", phase);

    ESP_LOGI(TAG, "FS_PROBE[%s]: end", phase);
}

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

    run_fs_probe("after_fs_init_before_seed");

    seed_test_files();

    run_fs_probe("after_seed_before_shell_init");

    ESP_ERROR_CHECK(shell_init());

    run_fs_probe("after_shell_init");

    ESP_LOGI(TAG, "SHELL_READY");
}
