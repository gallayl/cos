#pragma once

#include <stdint.h>

/**
 * Reset mock LEDC state.
 */
void mock_ledc_reset(void);

/**
 * Get the last duty value set for a given channel (0-7).
 * Returns 0 if channel never set.
 */
uint32_t mock_ledc_get_duty(int channel);
