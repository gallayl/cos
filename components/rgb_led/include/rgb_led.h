#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize the RGB LED hardware (LEDC PWM), load saved color
     * from NVS (defaulting to off), apply it, and register shell commands.
     */
    esp_err_t rgb_led_init(void);

    /**
     * Set the RGB LED color and persist it to NVS.
     * Each channel is 0-255 (0 = off, 255 = full brightness).
     */
    esp_err_t rgb_led_set_color(uint8_t r, uint8_t g, uint8_t b);

    /** Get the current RGB LED color. Any output pointer may be NULL. */
    void rgb_led_get_color(uint8_t *r, uint8_t *g, uint8_t *b);

    /** Turn the RGB LED off and persist. Shorthand for rgb_led_set_color(0, 0, 0). */
    esp_err_t rgb_led_off(void);

#ifdef __cplusplus
}
#endif
