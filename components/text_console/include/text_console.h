#pragma once

#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize the text console on the display.
     *
     * Sets up the character grid, hooks stdout and ESP_LOG output,
     * and clears the screen. Must be called after display_init().
     */
    esp_err_t text_console_init(void);

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

    /**
     * @brief Resize the console to match the current display dimensions.
     *
     * Recalculates cols/rows from display width/height, clears the screen,
     * and resets the cursor. Call after changing display rotation.
     */
    void text_console_resize(void);

    /** Register the display_mode shell command. */
    void text_console_register_commands(void);

#ifdef __cplusplus
}
#endif
