#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "display.h"

/**
 * Reset all mock display state and call counters.
 */
void mock_display_reset(void);

/**
 * Set the rotation the mock display will report.
 */
void mock_display_set_rotation(uint8_t rotation);

/**
 * Set the calibration data that display_calibrate_touch will produce.
 */
void mock_display_set_cal_result(const uint16_t cal_data[DISPLAY_CAL_DATA_LEN]);

/**
 * Get the last calibration data passed to display_set_touch_calibration.
 */
const uint16_t *mock_display_get_applied_cal(void);

/** Get number of times display_calibrate_touch was called. */
int mock_display_calibrate_call_count(void);

/** Get number of times display_set_touch_calibration was called. */
int mock_display_set_cal_call_count(void);

/** Get number of times display_fill_screen was called. */
int mock_display_fill_screen_call_count(void);
