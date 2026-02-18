#pragma once

#include "esp_event.h"

/** Reset mock event state. */
void mock_esp_event_reset(void);

/**
 * Fire the registered event handler with the given event_base, event_id,
 * and event_data. Useful for simulating WiFi/IP events in tests.
 */
void mock_esp_event_fire(esp_event_base_t event_base, int32_t event_id, void *event_data);
