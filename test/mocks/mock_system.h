#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void mock_system_reset(void);

void mock_system_set_free_heap(uint32_t bytes);
void mock_system_set_min_free_heap(uint32_t bytes);
void mock_system_set_total_heap(size_t bytes);
void mock_system_set_max_alloc(size_t bytes);
void mock_system_set_flash_size(uint32_t bytes);

/** Set the mock value returned by esp_timer_get_time(). */
void mock_system_set_uptime_us(int64_t us);

/** Check whether esp_restart() was called. */
bool mock_system_restart_called(void);
