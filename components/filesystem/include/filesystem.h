#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define VFS_PATH_MAX 128
#define VFS_NAME_MAX 64
#define VFS_ENTRIES_MAX 64

    /** Single directory entry returned by vfs_list_dir(). */
    typedef struct
    {
        char name[VFS_NAME_MAX]; /**< Entry name (filename or subdirectory). */
        size_t size;             /**< File size in bytes (0 for directories). */
        bool is_dir;             /**< True if the entry is a directory. */
        time_t mtime;            /**< Last modification time (epoch seconds). */
    } vfs_dir_entry_t;

    /**
     * Initialize filesystems: mount LittleFS, attempt SD card auto-mount.
     */
    esp_err_t filesystem_init(void);

    /* --- Flash (LittleFS) --- */
    size_t flash_get_total_bytes(void);
    size_t flash_get_used_bytes(void);
    esp_err_t flash_format(void);

    /* --- SD card --- */
    esp_err_t sdcard_mount(void);
    esp_err_t sdcard_unmount(void);
    bool sdcard_is_mounted(void);
    const char *sdcard_get_type_name(void);
    uint64_t sdcard_get_total_bytes(void);
    uint64_t sdcard_get_used_bytes(void);

    /* --- VFS path resolution --- */
    esp_err_t vfs_resolve_path(const char *virtual_path, char *real_path, size_t len);

    /* --- VFS file operations (accept virtual paths) --- */
    esp_err_t vfs_list_dir(const char *path, vfs_dir_entry_t *entries, size_t max_entries, size_t *count);
    esp_err_t vfs_read_file(const char *path, char *buf, size_t buf_size, size_t *bytes_read);
    esp_err_t vfs_write_file(const char *path, const char *data, size_t len);
    esp_err_t vfs_mkdir(const char *path);
    esp_err_t vfs_remove(const char *path);
    bool vfs_exists(const char *path);
    bool vfs_is_directory(const char *path);

#ifdef __cplusplus
}
#endif
