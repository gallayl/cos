#pragma once

/**
 * Reset the in-memory NVS fake.
 * Call before each test to ensure isolation.
 */
void mock_nvs_reset(void);

/**
 * Configure NVS mock to fail the next operation with the given error.
 * After one call returns this error, behaviour reverts to normal.
 */
void mock_nvs_set_next_error(int err);
