#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define BRIGHTNESS_MODE_MANUAL 0
#define BRIGHTNESS_MODE_AUTO 1

    /**
     * Initialize brightness manager: load settings from NVS, apply
     * brightness and rotation, and register shell commands.
     * Must be called after display_init(), light_sensor_init(), and calibration_init().
     */
    esp_err_t brightness_init(void);

    /**
     * Set manual brightness (0-255), persist to NVS, and disable auto mode.
     */
    esp_err_t brightness_set(uint8_t value);

    /** Get the current effective brightness (0-255). */
    uint8_t brightness_get(void);

    /** Enable or disable auto-brightness mode, persist to NVS. */
    esp_err_t brightness_set_auto(bool on);

    /** Query whether auto-brightness mode is active. */
    bool brightness_is_auto(void);

    /**
     * Map a raw ADC value (0-4095) to a brightness level (0-255).
     * Exposed for unit testing.
     */
    uint8_t brightness_map_adc(int adc_value);

#ifdef __cplusplus
}
#endif
