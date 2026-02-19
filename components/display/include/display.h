#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_CAL_DATA_LEN 8

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize the display, backlight, and touch controller.
     * Must be called once before any other display function.
     */
    esp_err_t display_init(void);

    /** Get the current display rotation (0-7). */
    uint8_t display_get_rotation(void);

    /** Set display rotation (0-7). */
    esp_err_t display_set_rotation(uint8_t rotation);

    /**
     * Run the interactive touch calibration.
     * Blocks until the user touches all calibration points.
     * Fills cal_data with 8 uint16_t calibration values.
     */
    esp_err_t display_calibrate_touch(uint16_t cal_data[DISPLAY_CAL_DATA_LEN]);

    /** Apply previously stored touch calibration data. */
    esp_err_t display_set_touch_calibration(const uint16_t cal_data[DISPLAY_CAL_DATA_LEN]);

    /** Set backlight brightness (0 = off, 255 = max). */
    esp_err_t display_set_brightness(uint8_t brightness);

    /** Get current backlight brightness (0-255). */
    uint8_t display_get_brightness(void);

    /** Fill entire screen with a 16-bit RGB565 color. */
    void display_fill_screen(uint16_t color);

    /** Fill a rectangle at (x, y) with dimensions w x h using a 16-bit RGB565 color. */
    void display_fill_rect(int x, int y, int w, int h, uint16_t color);

    /** Draw a null-terminated string at (x, y) with foreground/background colors. */
    void display_draw_text(int x, int y, const char *text, uint16_t fg, uint16_t bg);

    /**
     * @brief Draw a single character at (x, y) with foreground/background colors.
     *
     * @param x Pixel x-coordinate.
     * @param y Pixel y-coordinate.
     * @param c Character to draw.
     * @param fg Foreground color (RGB565).
     * @param bg Background color (RGB565).
     */
    void display_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg);

    /** Get the current display width in pixels (accounts for rotation). */
    int display_get_width(void);

    /** Get the current display height in pixels (accounts for rotation). */
    int display_get_height(void);

    /** Set the text font by LovyanGFX font ID. */
    void display_set_text_font(uint8_t font_id);

    /**
     * @brief Begin a batched draw transaction (acquire SPI bus).
     *
     * Call before a series of draw operations, paired with display_end_write().
     * Avoids per-call SPI bus acquire/release overhead.
     */
    void display_start_write(void);

    /** End a batched draw transaction (release SPI bus). */
    void display_end_write(void);

    /**
     * @brief Render a row of characters and push to the display in one DMA transfer.
     *
     * Uses an internal row-sized sprite to batch all characters, avoiding
     * per-character SPI overhead. Much faster than individual display_draw_char()
     * calls for full-row updates.
     *
     * @param y      Pixel y-coordinate of the row's top edge.
     * @param chars  Array of characters to draw (count elements).
     * @param fg     Array of per-character foreground colors (RGB565, count elements).
     * @param count  Number of characters in the row.
     * @param bg     Background color (RGB565).
     */
    void display_draw_text_row(int y, const char *chars, const uint16_t *fg, int count, uint16_t bg);

    /** Wait for all pending display DMA transfers to complete. */
    void display_wait(void);

#ifdef __cplusplus
}
#endif
