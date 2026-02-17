#include "filesystem.h"
#include "flash.h"
#include "sdcard.h"

#include "esp_log.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

static const char *const TAG = "vfs";

esp_err_t filesystem_init(void)
{
    esp_err_t ret = flash_init();
    if (ret != ESP_OK)
    {
        return ret;
    }

    esp_err_t sd_ret = sdcard_init();
    if (sd_ret != ESP_OK)
    {
        ESP_LOGI(TAG, "SD card not available (%s)", esp_err_to_name(sd_ret));
    }

    return ESP_OK;
}

/* ---- Path resolution ---- */

esp_err_t vfs_resolve_path(const char *virtual_path, char *real_path, size_t len)
{
    if (virtual_path == NULL || real_path == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (strncmp(virtual_path, "/flash", 6) == 0)
    {
        const char *rest = virtual_path + 6;
        if (rest[0] == '\0')
        {
            rest = "/";
        }
        int written = snprintf(real_path, len, "%s%s", FLASH_MOUNT_POINT, rest);
        if (written < 0 || (size_t)written >= len)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        return ESP_OK;
    }

    if (strncmp(virtual_path, "/sdcard", 7) == 0)
    {
        const char *rest = virtual_path + 7;
        if (rest[0] == '\0')
        {
            rest = "/";
        }
        int written = snprintf(real_path, len, "%s%s", SDCARD_MOUNT_POINT, rest);
        if (written < 0 || (size_t)written >= len)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

/* ---- File operations ---- */

esp_err_t vfs_list_dir(const char *path, vfs_dir_entry_t *entries, size_t max_entries, size_t *count)
{
    if (path == NULL || entries == NULL || count == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *count = 0;

    /* Virtual root: list available filesystems */
    if (strcmp(path, "/") == 0)
    {
        if (max_entries < 1)
        {
            return ESP_ERR_NO_MEM;
        }
        strncpy(entries[*count].name, "flash", VFS_NAME_MAX);
        entries[*count].size = 0;
        entries[*count].is_dir = true;
        entries[*count].mtime = 0;
        (*count)++;

        if (sdcard_is_mounted() && *count < max_entries)
        {
            strncpy(entries[*count].name, "sdcard", VFS_NAME_MAX);
            entries[*count].size = 0;
            entries[*count].is_dir = true;
            entries[*count].mtime = 0;
            (*count)++;
        }
        return ESP_OK;
    }

    char real_path[VFS_PATH_MAX];
    esp_err_t err = vfs_resolve_path(path, real_path, sizeof(real_path));
    if (err != ESP_OK)
    {
        return err;
    }

    DIR *dir = opendir(real_path);
    if (dir == NULL)
    {
        ESP_LOGD(TAG, "Cannot open directory: %s", real_path);
        return ESP_ERR_NOT_FOUND;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && *count < max_entries)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        strncpy(entries[*count].name, entry->d_name, VFS_NAME_MAX - 1);
        entries[*count].name[VFS_NAME_MAX - 1] = '\0';

        char entry_path[VFS_PATH_MAX];
        size_t rp_len = strlen(real_path);
        const char *sep = (rp_len > 0 && real_path[rp_len - 1] == '/') ? "" : "/";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(entry_path, sizeof(entry_path), "%s%s%s", real_path, sep, entry->d_name);
#pragma GCC diagnostic pop

        struct stat st;
        if (stat(entry_path, &st) == 0)
        {
            entries[*count].is_dir = S_ISDIR(st.st_mode);
            entries[*count].size = (size_t)st.st_size;
            entries[*count].mtime = st.st_mtime;
        }
        else
        {
            entries[*count].is_dir = (entry->d_type == DT_DIR);
            entries[*count].size = 0;
            entries[*count].mtime = 0;
        }

        (*count)++;
    }

    closedir(dir);
    return ESP_OK;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- buf is an output buffer filled by fread()
esp_err_t vfs_read_file(const char *path, char *buf, size_t buf_size, size_t *bytes_read)
{
    if (path == NULL || buf == NULL || bytes_read == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *bytes_read = 0;

    char real_path[VFS_PATH_MAX];
    esp_err_t err = vfs_resolve_path(path, real_path, sizeof(real_path));
    if (err != ESP_OK)
    {
        return err;
    }

    FILE *f = fopen(real_path, "r");
    if (f == NULL)
    {
        return ESP_ERR_NOT_FOUND;
    }

    *bytes_read = fread(buf, 1, buf_size, f);
    fclose(f);
    return ESP_OK;
}

esp_err_t vfs_write_file(const char *path, const char *data, size_t len)
{
    if (path == NULL || (data == NULL && len > 0))
    {
        return ESP_ERR_INVALID_ARG;
    }

    char real_path[VFS_PATH_MAX];
    esp_err_t err = vfs_resolve_path(path, real_path, sizeof(real_path));
    if (err != ESP_OK)
    {
        return err;
    }

    FILE *f = fopen(real_path, "w");
    if (f == NULL)
    {
        return ESP_FAIL;
    }

    if (len > 0)
    {
        size_t written = fwrite(data, 1, len, f);
        if (written != len)
        {
            fclose(f);
            return ESP_FAIL;
        }
    }

    fclose(f);
    return ESP_OK;
}

esp_err_t vfs_mkdir(const char *path)
{
    if (path == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char real_path[VFS_PATH_MAX];
    esp_err_t err = vfs_resolve_path(path, real_path, sizeof(real_path));
    if (err != ESP_OK)
    {
        return err;
    }

    if (mkdir(real_path, 0775) != 0)
    {
        ESP_LOGE(TAG, "mkdir failed: %s", real_path);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t vfs_remove(const char *path)
{
    if (path == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char real_path[VFS_PATH_MAX];
    esp_err_t err = vfs_resolve_path(path, real_path, sizeof(real_path));
    if (err != ESP_OK)
    {
        return err;
    }

    struct stat st;
    if (stat(real_path, &st) != 0)
    {
        return ESP_ERR_NOT_FOUND;
    }

    if (S_ISDIR(st.st_mode))
    {
        if (rmdir(real_path) != 0)
        {
            ESP_LOGE(TAG, "rmdir failed: %s", real_path);
            return ESP_FAIL;
        }
    }
    else
    {
        if (unlink(real_path) != 0)
        {
            ESP_LOGE(TAG, "unlink failed: %s", real_path);
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

bool vfs_exists(const char *path)
{
    if (path == NULL)
    {
        return false;
    }

    if (strcmp(path, "/") == 0 || strcmp(path, "/flash") == 0)
    {
        return true;
    }
    if (strcmp(path, "/sdcard") == 0)
    {
        return sdcard_is_mounted();
    }

    char real_path[VFS_PATH_MAX];
    if (vfs_resolve_path(path, real_path, sizeof(real_path)) != ESP_OK)
    {
        return false;
    }

    struct stat st;
    return stat(real_path, &st) == 0;
}

bool vfs_is_directory(const char *path)
{
    if (path == NULL)
    {
        return false;
    }

    if (strcmp(path, "/") == 0 || strcmp(path, "/flash") == 0)
    {
        return true;
    }
    if (strcmp(path, "/sdcard") == 0)
    {
        return sdcard_is_mounted();
    }

    char real_path[VFS_PATH_MAX];
    if (vfs_resolve_path(path, real_path, sizeof(real_path)) != ESP_OK)
    {
        return false;
    }

    struct stat st;
    if (stat(real_path, &st) != 0)
    {
        return false;
    }
    return S_ISDIR(st.st_mode);
}
