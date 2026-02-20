#pragma once

#include "esp_err.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize the shell: register all commands and start the
     * shared input subsystem (UART + BT keyboard).
     * The default working directory is /flash.
     */
    esp_err_t shell_init(void);

    /**
     * Feed raw input bytes into the shell's shared line editor.
     * Thread-safe -- can be called from any task (e.g. BT keyboard callback).
     * Matches the bt_keyboard_cb_t signature.
     */
    void shell_feed_input(const char *data, size_t len);

    /**
     * Get the current virtual working directory (e.g. "/flash", "/sdcard/data").
     */
    const char *shell_get_cwd(void);

    /**
     * Set the current virtual working directory. Updates the prompt.
     * Returns ESP_ERR_NOT_FOUND if the path does not exist or is not a directory.
     */
    esp_err_t shell_set_cwd(const char *path);

    /**
     * Get the current prompt string (e.g. "COS/flash> ").
     */
    const char *shell_get_prompt(void);

    /**
     * Build an absolute virtual path from a possibly-relative path and the current cwd.
     * Handles ".", "..", absolute paths, and relative paths.
     * Result is written into abs_path (must be at least VFS_PATH_MAX bytes).
     */
    esp_err_t shell_resolve_relative(const char *input, char *abs_path, size_t len);

#ifdef __cplusplus
}
#endif
