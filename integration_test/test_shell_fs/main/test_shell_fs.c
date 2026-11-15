#include "filesystem.h"
#include "shell.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "generated_test_scripts.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *const TAG = "test_shell_fs";
static char s_large_file[1200];

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
    ESP_ERROR_CHECK(vfs_write_file("/flash/berry_api.be", (const char *)shell_fs_fixture_berry_api_be, shell_fs_fixture_berry_api_be_len));
    ESP_ERROR_CHECK(vfs_write_file("/flash/berry_net.be", (const char *)shell_fs_fixture_berry_net_be, shell_fs_fixture_berry_net_be_len));
    ESP_ERROR_CHECK(vfs_write_file("/flash/berry_input.be", (const char *)shell_fs_fixture_berry_input_be, shell_fs_fixture_berry_input_be_len));
    ESP_ERROR_CHECK(vfs_write_file("/flash/autoexec.be", (const char *)shell_fs_fixture_autoexec_be, shell_fs_fixture_autoexec_be_len));
    ESP_ERROR_CHECK(vfs_write_file("/flash/ping.be", (const char *)shell_fs_fixture_ping_be, shell_fs_fixture_ping_be_len));
    ESP_ERROR_CHECK(vfs_write_file("/flash/nslookup.be", (const char *)shell_fs_fixture_nslookup_be, shell_fs_fixture_nslookup_be_len));

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
