#pragma once

#include "freertos/event_groups.h"

/** Reset mock FreeRTOS state (event group bits). */
void mock_freertos_reset(void);

/** Directly set event group bits (simulate event handler setting them). */
void mock_freertos_set_bits(EventBits_t bits);

/** Directly clear event group bits. */
void mock_freertos_clear_bits(EventBits_t bits);

/** Get current event group bits. */
EventBits_t mock_freertos_get_bits(void);
