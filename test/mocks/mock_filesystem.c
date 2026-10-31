#include "mock_filesystem.h"
#include "esp_err.h"
#include "filesystem.h"

#include <stdio.h>
#include <string.h>

#define MOCK_MAX_DIRS 32
#define MOCK_MAX_FILES 64

static bool s_sd_mounted = false;
static char s_directories[MOCK_MAX_DIRS][VFS_PATH_MAX];
static int s_dir_count = 0;
static char s_files[MOCK_MAX_FILES][VFS_PATH_MAX];
static int s_file_count = 0;

static bool path_in_list(const char paths[][VFS_PATH_MAX], int count, const char *path)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(paths[i], path) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool path_exists(const char *path)
{
    return path_in_list((const char (*)[VFS_PATH_MAX])s_directories, s_dir_count, path) ||
           path_in_list((const char (*)[VFS_PATH_MAX])s_files, s_file_count, path);
}

static void add_entry(vfs_dir_entry_t *entries, size_t max_entries, size_t *count, const char *name, bool is_dir)
{
    if (*count >= max_entries)
    {
        return;
    }
    for (size_t i = 0; i < *count; i++)
    {
        if (strcmp(entries[i].name, name) == 0)
        {
            if (is_dir)
            {
                entries[i].is_dir = true;
            }
            return;
        }
    }
    strncpy(entries[*count].name, name, VFS_NAME_MAX - 1);
    entries[*count].name[VFS_NAME_MAX - 1] = '\0';
    entries[*count].size = is_dir ? 0 : 1;
    entries[*count].is_dir = is_dir;
    entries[*count].mtime = 0;
    (*count)++;
}

static void list_children(const char *parent, const char paths[][VFS_PATH_MAX], int path_count, bool is_dir,
                          vfs_dir_entry_t *entries, size_t max_entries, size_t *count)
{
    size_t parent_len = strlen(parent);
    const char *child_start = NULL;
    for (int i = 0; i < path_count && *count < max_entries; i++)
    {
        if (strcmp(parent, "/") == 0)
        {
            if (paths[i][0] != '/' || paths[i][1] == '\0')
            {
                continue;
            }
            child_start = paths[i] + 1;
        }
        else
        {
            if (strncmp(paths[i], parent, parent_len) != 0 || paths[i][parent_len] != '/')
            {
                continue;
            }
            child_start = paths[i] + parent_len + 1;
        }
        if (child_start[0] == '\0')
        {
            continue;
        }

        const char *separator = strchr(child_start, '/');
        char name[VFS_NAME_MAX];
        size_t name_len = separator != NULL ? (size_t)(separator - child_start) : strlen(child_start);
        if (name_len == 0 || name_len >= sizeof(name))
        {
            continue;
        }
        memcpy(name, child_start, name_len);
        name[name_len] = '\0';
        add_entry(entries, max_entries, count, name, is_dir || separator != NULL);
    }
}

void mock_filesystem_reset(void)
{
    s_sd_mounted = false;
    s_dir_count = 0;
    s_file_count = 0;
    memset(s_directories, 0, sizeof(s_directories));
    memset(s_files, 0, sizeof(s_files));

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
    if (s_dir_count < MOCK_MAX_DIRS && !path_in_list((const char (*)[VFS_PATH_MAX])s_directories, s_dir_count, path))
    {
        strncpy(s_directories[s_dir_count], path, VFS_PATH_MAX - 1);
        s_dir_count++;
    }
}

void mock_filesystem_add_file(const char *path)
{
    if (s_file_count < MOCK_MAX_FILES && !path_in_list((const char (*)[VFS_PATH_MAX])s_files, s_file_count, path))
    {
        strncpy(s_files[s_file_count], path, VFS_PATH_MAX - 1);
        s_file_count++;
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
    if (path == NULL || entries == NULL || count == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *count = 0;
    if (!vfs_is_directory(path))
    {
        return ESP_ERR_NOT_FOUND;
    }
    list_children(path, (const char (*)[VFS_PATH_MAX])s_directories, s_dir_count, true, entries, max_entries, count);
    list_children(path, (const char (*)[VFS_PATH_MAX])s_files, s_file_count, false, entries, max_entries, count);
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

esp_err_t vfs_read_file_stream(const char *path, vfs_read_callback_t callback, void *context)
{
    (void)path;
    (void)callback;
    (void)context;
    return ESP_ERR_NOT_FOUND;
}

esp_err_t vfs_write_file(const char *path, const char *data, size_t len)
{
    (void)data;
    (void)len;
    if (path == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!path_exists(path))
    {
        mock_filesystem_add_file(path);
    }
    return ESP_OK;
}

esp_err_t vfs_copy_file(const char *source, const char *destination)
{
    if (source == NULL || destination == NULL ||
        !path_in_list((const char (*)[VFS_PATH_MAX])s_files, s_file_count, source))
    {
        return ESP_ERR_NOT_FOUND;
    }
    if (path_exists(destination))
    {
        return ESP_ERR_INVALID_STATE;
    }
    mock_filesystem_add_file(destination);
    return ESP_OK;
}

esp_err_t vfs_rename_file(const char *source, const char *destination)
{
    if (source == NULL || destination == NULL ||
        !path_in_list((const char (*)[VFS_PATH_MAX])s_files, s_file_count, source))
    {
        return ESP_ERR_NOT_FOUND;
    }
    if (path_exists(destination))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if ((strncmp(source, "/flash/", 7) == 0) != (strncmp(destination, "/flash/", 7) == 0))
    {
        return ESP_ERR_NOT_SUPPORTED;
    }

    for (int i = 0; i < s_file_count; i++)
    {
        if (strcmp(s_files[i], source) == 0)
        {
            strncpy(s_files[i], destination, VFS_PATH_MAX - 1);
            s_files[i][VFS_PATH_MAX - 1] = '\0';
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t vfs_mkdir(const char *path)
{
    if (path == NULL || path_exists(path))
    {
        return ESP_FAIL;
    }
    mock_filesystem_add_directory(path);
    return ESP_OK;
}

esp_err_t vfs_remove(const char *path)
{
    if (path == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (int i = 0; i < s_file_count; i++)
    {
        if (strcmp(s_files[i], path) == 0)
        {
            memmove(&s_files[i], &s_files[i + 1], (size_t)(s_file_count - i - 1) * sizeof(s_files[0]));
            s_file_count--;
            return ESP_OK;
        }
    }
    for (int i = 0; i < s_dir_count; i++)
    {
        if (strcmp(s_directories[i], path) == 0 && strcmp(path, "/") != 0 && strcmp(path, "/flash") != 0)
        {
            for (int j = 0; j < s_dir_count; j++)
            {
                if (strncmp(s_directories[j], path, strlen(path)) == 0 && s_directories[j][strlen(path)] == '/')
                {
                    return ESP_FAIL;
                }
            }
            for (int j = 0; j < s_file_count; j++)
            {
                if (strncmp(s_files[j], path, strlen(path)) == 0 && s_files[j][strlen(path)] == '/')
                {
                    return ESP_FAIL;
                }
            }
            memmove(&s_directories[i], &s_directories[i + 1], (size_t)(s_dir_count - i - 1) * sizeof(s_directories[0]));
            s_dir_count--;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

bool vfs_exists(const char *path)
{
    if (path == NULL)
    {
        return false;
    }
    if (strcmp(path, "/sdcard") == 0 && !s_sd_mounted)
    {
        return false;
    }
    return path_exists(path);
}

bool vfs_is_directory(const char *path)
{
    if (path == NULL || (strcmp(path, "/sdcard") == 0 && !s_sd_mounted))
    {
        return false;
    }
    return path_in_list((const char (*)[VFS_PATH_MAX])s_directories, s_dir_count, path);
}
