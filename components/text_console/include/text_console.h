#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        TEXT_CONSOLE_LOG_ERROR,
        TEXT_CONSOLE_LOG_WARN,
        TEXT_CONSOLE_LOG_INFO,
    } text_console_log_level_t;

    /**
     * @brief Initialize the text console on the display.
     *
     * Sets up the character grid, hooks stdout and ESP_LOG output,
     * and clears the screen. Must be called after display_init().
     */
    esp_err_t text_console_init(void);

    /** Enable or disable rendering the console buffer on the display. */
    esp_err_t text_console_set_visible(bool visible);

    /**
     * @brief Deinitialize the text console.
     *
     * Unhooks stdout and ESP_LOG interception, freeing the display
     * for other use (e.g. graphics mode).
     */
    void text_console_deinit(void);

    /**
     * @brief Write raw data to the text console.
     *
     * Processes VT100 escape sequences (cursor movement, erase, SGR colors)
     * and renders the result on the display. Thread-safe.
     *
     * @param data Byte stream to process.
     * @param len Number of bytes.
     */
    void text_console_write(const char *data, size_t len);

    /** Clear the text console screen and reset cursor to top-left. */
    void text_console_clear(void);

    /** Release retained history without clearing the visible console grid. */
    esp_err_t text_console_release_scrollback(void);

    /** Set and persist the global ESP log threshold. */
    esp_err_t text_console_set_log_level(text_console_log_level_t level);

    /** Get the active global ESP log threshold. */
    text_console_log_level_t text_console_get_log_level(void);

    /** Get the printable name for a supported log threshold. */
    const char *text_console_log_level_name(text_console_log_level_t level);

    /**
     * @brief Resize the console to match the current display dimensions.
     *
     * Recalculates cols/rows from display width/height, clears the screen,
     * and resets the cursor. Call after changing display rotation.
     */
    void text_console_resize(void);

    /** Register clear, display_mode, and logging shell commands. */
    void text_console_register_commands(void);

    /**
     * @brief Receive a copy of bytes written to the console stdout hook.
     *
     * The listener runs on the task that wrote to stdout, after the bytes
     * have been forwarded to UART. It must return quickly. Nested stdout
     * from the same task is not delivered again.
     */
    typedef void (*text_console_output_listener_t)(const char *data, size_t len, void *context);

    /**
     * @brief Install or clear the stdout listener.
     * @param listener Callback, or NULL to clear it.
     * @param context Value passed through to the listener.
     */
    void text_console_set_output_listener(text_console_output_listener_t listener, void *context);

#ifdef __cplusplus
}
#endif
