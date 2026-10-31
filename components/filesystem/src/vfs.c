#include "filesystem.h"
#include "flash.h"
#include "sdcard.h"

#include "esp_log.h"
#include "sdkconfig.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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

#if CONFIG_COS_FILESYSTEM_DISABLE_SDCARD
    ESP_LOGI(TAG, "SD card initialization disabled by configuration");
#else
    esp_err_t sd_ret = sdcard_init();
    if (sd_ret != ESP_OK)
    {
        ESP_LOGI(TAG, "SD card not available (%s)", esp_err_to_name(sd_ret));
    }
#endif

    return ESP_OK;
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
        int written = snprintf(entry_path, sizeof(entry_path), "%s%s%s", real_path, sep, entry->d_name);

        struct stat st;
        if (written < 0 || (size_t)written >= sizeof(entry_path))
        {
            entries[*count].is_dir = (entry->d_type == DT_DIR);
            entries[*count].size = 0;
            entries[*count].mtime = 0;
            (*count)++;
            continue;
        }

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

esp_err_t vfs_read_file_stream(const char *path, vfs_read_callback_t callback, void *context)
{
    if (path == NULL || callback == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

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

    char buf[512];
    do
    {
        size_t bytes_read = fread(buf, 1, sizeof(buf), f);
        if (bytes_read > 0)
        {
            err = callback(buf, bytes_read, context);
            if (err != ESP_OK)
            {
                break;
            }
        }

        if (bytes_read < sizeof(buf))
        {
            if (ferror(f))
            {
                err = ESP_FAIL;
            }
            break;
        }
    } while (true);

    if (fclose(f) != 0 && err == ESP_OK)
    {
        err = ESP_FAIL;
    }
    return err;
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

static bool vfs_paths_share_mount(const char *first, const char *second)
{
    const char *first_mount_end = strchr(first + 1, '/');
    const char *second_mount_end = strchr(second + 1, '/');
    if (first_mount_end == NULL || second_mount_end == NULL)
    {
        return false;
    }

    size_t first_mount_len = (size_t)(first_mount_end - first);
    size_t second_mount_len = (size_t)(second_mount_end - second);
    if (first_mount_len != second_mount_len)
    {
        return false;
    }
    return strncmp(first, second, first_mount_len) == 0;
}

esp_err_t vfs_copy_file(const char *source, const char *destination)
{
    if (source == NULL || destination == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char source_path[VFS_PATH_MAX];
    char destination_path[VFS_PATH_MAX];
    esp_err_t err = vfs_resolve_path(source, source_path, sizeof(source_path));
    if (err != ESP_OK)
    {
        return err;
    }
    err = vfs_resolve_path(destination, destination_path, sizeof(destination_path));
    if (err != ESP_OK)
    {
        return err;
    }

    int source_fd = open(source_path, O_RDONLY);
    if (source_fd < 0)
    {
        return ESP_ERR_NOT_FOUND;
    }

    struct stat source_stat;
    if (fstat(source_fd, &source_stat) != 0)
    {
        close(source_fd);
        return ESP_FAIL;
    }
    if (!S_ISREG(source_stat.st_mode))
    {
        close(source_fd);
        return ESP_ERR_INVALID_ARG;
    }

    int destination_fd = open(destination_path, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (destination_fd < 0)
    {
        int open_error = errno;
        if (close(source_fd) != 0)
        {
            return ESP_FAIL;
        }
        return (open_error == EEXIST) ? ESP_ERR_INVALID_STATE : ESP_FAIL;
    }

    char buffer[512];
    while (err == ESP_OK)
    {
        ssize_t bytes_read = read(source_fd, buffer, sizeof(buffer));
        if (bytes_read < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            err = ESP_FAIL;
            break;
        }
        if (bytes_read == 0)
        {
            break;
        }

        ssize_t offset = 0;
        while (offset < bytes_read)
        {
            ssize_t bytes_written = write(destination_fd, buffer + offset, (size_t)(bytes_read - offset));
            if (bytes_written < 0 && errno == EINTR)
            {
                continue;
            }
            if (bytes_written <= 0)
            {
                err = ESP_FAIL;
                break;
            }
            offset += bytes_written;
        }
    }

    if (close(source_fd) != 0 && err == ESP_OK)
    {
        err = ESP_FAIL;
    }
    if (close(destination_fd) != 0 && err == ESP_OK)
    {
        err = ESP_FAIL;
    }
    if (err != ESP_OK && unlink(destination_path) != 0)
    {
        ESP_LOGE(TAG, "Failed to clean up partial copy: %s", destination_path);
    }

    return err;
}

esp_err_t vfs_rename_file(const char *source, const char *destination)
{
    if (source == NULL || destination == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char source_path[VFS_PATH_MAX];
    char destination_path[VFS_PATH_MAX];
    esp_err_t err = vfs_resolve_path(source, source_path, sizeof(source_path));
    if (err != ESP_OK)
    {
        return err;
    }
    err = vfs_resolve_path(destination, destination_path, sizeof(destination_path));
    if (err != ESP_OK)
    {
        return err;
    }

    if (!vfs_paths_share_mount(source_path, destination_path))
    {
        return ESP_ERR_NOT_SUPPORTED;
    }

    struct stat source_stat;
    if (stat(source_path, &source_stat) != 0)
    {
        return ESP_ERR_NOT_FOUND;
    }
    if (!S_ISREG(source_stat.st_mode))
    {
        return ESP_ERR_INVALID_ARG;
    }

    struct stat destination_stat;
    if (stat(destination_path, &destination_stat) == 0)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (errno != ENOENT)
    {
        return ESP_FAIL;
    }

    return (rename(source_path, destination_path) == 0) ? ESP_OK : ESP_FAIL;
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
