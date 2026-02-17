#include "mock_filesystem.h"
#include "filesystem.h"
#include "esp_err.h"

#include <string.h>
#include <stdio.h>

#define MOCK_MAX_DIRS 32

static bool s_sd_mounted = false;
static char s_directories[MOCK_MAX_DIRS][VFS_PATH_MAX];
static int s_dir_count = 0;

void mock_filesystem_reset(void)
{
    s_sd_mounted = false;
    s_dir_count = 0;
    memset(s_directories, 0, sizeof(s_directories));

    /* Default directories always present */
    mock_filesystem_add_directory("/");
    mock_filesystem_add_directory("/flash");
}

void mock_filesystem_set_sd_mounted(bool mounted)
{
    s_sd_mounted = mounted;
    if (mounted)
    {
        mock_filesystem_add_directory("/sdcard");
    }
}

void mock_filesystem_add_directory(const char *path)
{
    if (s_dir_count < MOCK_MAX_DIRS)
    {
        strncpy(s_directories[s_dir_count], path, VFS_PATH_MAX - 1);
        s_dir_count++;
    }
}

/* --- ESP-IDF utility stubs --- */

const char *esp_err_to_name(esp_err_t code)
{
    (void)code;
    return "MOCK_ERR";
}

/* --- Filesystem API stubs --- */

esp_err_t filesystem_init(void)
{
    return ESP_OK;
}

size_t flash_get_total_bytes(void)
{
    return 524288;
}

size_t flash_get_used_bytes(void)
{
    return 0;
}

esp_err_t flash_format(void)
{
    return ESP_OK;
}

esp_err_t sdcard_mount(void)
{
    s_sd_mounted = true;
    return ESP_OK;
}

esp_err_t sdcard_unmount(void)
{
    s_sd_mounted = false;
    return ESP_OK;
}

bool sdcard_is_mounted(void)
{
    return s_sd_mounted;
}

const char *sdcard_get_type_name(void)
{
    return s_sd_mounted ? "SDHC" : "NONE";
}

uint64_t sdcard_get_total_bytes(void)
{
    return s_sd_mounted ? 4294967296ULL : 0;
}

uint64_t sdcard_get_used_bytes(void)
{
    return 0;
}

esp_err_t vfs_list_dir(const char *path, vfs_dir_entry_t *entries, size_t max_entries, size_t *count)
{
    (void)path;
    (void)entries;
    (void)max_entries;
    if (count)
    {
        *count = 0;
    }
    return ESP_OK;
}

esp_err_t vfs_read_file(const char *path, char *buf, size_t buf_size, size_t *bytes_read)
{
    (void)path;
    (void)buf;
    (void)buf_size;
    if (bytes_read)
    {
        *bytes_read = 0;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t vfs_write_file(const char *path, const char *data, size_t len)
{
    (void)path;
    (void)data;
    (void)len;
    return ESP_OK;
}

esp_err_t vfs_mkdir(const char *path)
{
    (void)path;
    return ESP_OK;
}

esp_err_t vfs_remove(const char *path)
{
    (void)path;
    return ESP_OK;
}

bool vfs_exists(const char *path)
{
    for (int i = 0; i < s_dir_count; i++)
    {
        if (strcmp(s_directories[i], path) == 0)
        {
            return true;
        }
    }
    return false;
}

bool vfs_is_directory(const char *path)
{
    return vfs_exists(path);
}
