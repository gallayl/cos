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

    /** Draw a null-terminated string at (x, y) with foreground/background colors. */
    void display_draw_text(int x, int y, const char *text, uint16_t fg, uint16_t bg);

    /** Wait for all pending display DMA transfers to complete. */
    void display_wait(void);

#ifdef __cplusplus
}
#endif
