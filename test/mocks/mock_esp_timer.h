#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * Reset mock esp_timer state.
 */
void mock_esp_timer_reset(void);

/**
 * Check if a periodic timer is currently running.
 */
bool mock_esp_timer_is_running(void);

/**
 * Get the period (in microseconds) of the last started periodic timer.
 */
uint64_t mock_esp_timer_get_period_us(void);

/**
 * Manually fire the registered timer callback (simulates a timer tick).
 */
void mock_esp_timer_fire(void);
