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

    /** Foreground color for one console write. The grid background stays black. */
    typedef enum
    {
        TEXT_CONSOLE_COLOR_CURRENT, /**< Keep the foreground already selected. */
        TEXT_CONSOLE_COLOR_DEFAULT, /**< White, the console default. */
        TEXT_CONSOLE_COLOR_BLACK,
        TEXT_CONSOLE_COLOR_RED,
        TEXT_CONSOLE_COLOR_GREEN,
        TEXT_CONSOLE_COLOR_YELLOW,
        TEXT_CONSOLE_COLOR_BLUE,
        TEXT_CONSOLE_COLOR_MAGENTA,
        TEXT_CONSOLE_COLOR_CYAN,
        TEXT_CONSOLE_COLOR_WHITE,
    } text_console_color_t;

/** Longest string accepted by text_console_write_text() and text_console_write_at(). */
#define TEXT_CONSOLE_WRITE_MAX 1024

/** Smallest integer scale of the 6x8 console cell. */
#define TEXT_CONSOLE_FONT_SIZE_MIN 1

/** Largest integer scale of the 6x8 console cell. */
#define TEXT_CONSOLE_FONT_SIZE_MAX 3

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

    /**
     * @brief Read the active character-grid size.
     * @param cols Receives the number of columns.
     * @param rows Receives the number of rows.
     * @return ESP_OK, ESP_ERR_INVALID_ARG, or ESP_ERR_INVALID_STATE.
     */
    esp_err_t text_console_get_size(int *cols, int *rows);

    /**
     * @brief Read the cursor in 0-based character cells.
     *
     * The origin is the top-left cell. `x` is the column and `y` is the row.
     */
    esp_err_t text_console_get_cursor(int *x, int *y);

    /**
     * @brief Move the cursor to a 0-based character cell.
     * @return ESP_OK, or ESP_ERR_INVALID_ARG when the cell is outside the grid.
     */
    esp_err_t text_console_set_cursor(int x, int y);

    /**
     * @brief Write at the cursor and advance it.
     *
     * Newline, carriage return, tab, and backspace behave as they do for stdout.
     * The color applies only to this string. Text longer than TEXT_CONSOLE_WRITE_MAX
     * is rejected.
     */
    esp_err_t text_console_write_text(const char *text, text_console_color_t color);

    /**
     * @brief Move to a cell and write without wrapping onto the next row.
     *
     * Printable characters that do not fit are dropped. Control characters are skipped.
     * The cursor is left after the last character written.
     */
    esp_err_t text_console_write_at(int x, int y, const char *text, text_console_color_t color);

    /**
     * @brief Scale the fixed 6x8 cell and rebuild the grid.
     *
     * Size 1 is 6x8 pixels, 2 is 12x16, and 3 is 18x24. The visible grid is cleared
     * because the column and row counts change.
     */
    esp_err_t text_console_set_font_size(uint8_t size);

    /** Read the active cell scale, from TEXT_CONSOLE_FONT_SIZE_MIN to MAX. */
    esp_err_t text_console_get_font_size(uint8_t *size);

#ifdef __cplusplus
}
#endif
