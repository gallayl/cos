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
        time_t ctime;            /**< Creation/status-change time (epoch seconds). */
    } vfs_dir_entry_t;

    /** Callback invoked with each bounded chunk read by vfs_read_file_stream(). */
    typedef esp_err_t (*vfs_read_callback_t)(const void *data, size_t len, void *context);

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

    /**
     * @brief Join a script-relative or absolute path and normalize `.` and `..`.
     *
     * Absolute paths start with `/`. Relative paths start with `./` and are
     * resolved from the directory that contains `script_file`. Any other form
     * is rejected. `script_file` may be NULL when `input` is absolute.
     *
     * @param script_file Virtual path of the running script, or NULL.
     * @param input Path supplied by the caller.
     * @param resolved Output buffer for the normalized absolute virtual path.
     * @param len Size of `resolved`, including the terminator.
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG when the form is rejected or `..` climbs above `/`.
     * @return ESP_ERR_INVALID_STATE when a relative path has no script file.
     * @return ESP_ERR_INVALID_SIZE when the path does not fit.
     */
    esp_err_t vfs_compose_script_path(const char *script_file, const char *input, char *resolved, size_t len);

    /**
     * @brief Report whether a normalized path is on the virtual filesystem.
     *
     * Accepts `/`, `/flash`, `/sdcard`, and their descendants.
     *
     * @param path Normalized absolute virtual path.
     * @return True when `path` is one of those locations.
     */
    bool vfs_is_virtual_path(const char *path);

    /* --- VFS file operations (accept virtual paths) --- */
    esp_err_t vfs_list_dir(const char *path, vfs_dir_entry_t *entries, size_t max_entries, size_t *count);
    esp_err_t vfs_read_file(const char *path, char *buf, size_t buf_size, size_t *bytes_read);
    /**
     * @brief Stream a file in bounded chunks without buffering the whole file.
     *
     * The callback runs synchronously; its data is only valid until it returns.
     * Return ESP_OK to continue or another error to stop and propagate that error.
     *
     * @param path Virtual file path.
     * @param callback Called for each chunk read.
     * @param context Opaque value passed to each callback invocation.
     * @return ESP_OK on success or an error from path resolution, reading, or callback.
     */
    esp_err_t vfs_read_file_stream(const char *path, vfs_read_callback_t callback, void *context);
    esp_err_t vfs_write_file(const char *path, const char *data, size_t len);
    /**
     * @brief Copy a regular file using bounded memory.
     *
     * The destination must not already exist. On failure, a partially written
     * destination is removed.
     */
    esp_err_t vfs_copy_file(const char *source, const char *destination);
    /**
     * @brief Rename a file or directory without replacing an existing destination.
     *
     * Source and destination must be on the same mounted filesystem.
     * A directory can be renamed only within that filesystem.
     */
    esp_err_t vfs_rename_file(const char *source, const char *destination);
    esp_err_t vfs_mkdir(const char *path);
    esp_err_t vfs_remove(const char *path);
    bool vfs_exists(const char *path);
    bool vfs_is_directory(const char *path);

    /**
     * @brief Read the kind and size of a virtual path.
     *
     * Directories report a size of 0. `/` and `/flash` are directories.
     * `/sdcard` is a directory only while the card is mounted.
     *
     * @param path Virtual path.
     * @param is_dir Set when the path is a directory.
     * @param size File size in bytes. Directories report 0.
     * @return ESP_OK on success.
     * @return ESP_ERR_NOT_FOUND when the path is missing.
     * @return ESP_ERR_INVALID_ARG when an argument is NULL.
     */
    esp_err_t vfs_stat(const char *path, bool *is_dir, size_t *size);

#ifdef __cplusplus
}
#endif
