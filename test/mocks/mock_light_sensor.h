#pragma once

/**
 * Reset mock light sensor state.
 */
void mock_light_sensor_reset(void);

/**
 * Set the value that light_sensor_read() will return.
 */
void mock_light_sensor_set_value(int value);
