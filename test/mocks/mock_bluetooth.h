#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

/** Reset all mock bluetooth state. */
void mock_bluetooth_reset(void);

/* --- Configure state returned by query functions --- */

void mock_bluetooth_set_enabled(bool enabled);
void mock_bluetooth_set_enabling(bool enabling);
void mock_bluetooth_set_hid_ready(bool ready);
void mock_bluetooth_set_connected(bool connected);
void mock_bluetooth_set_device_name(const char *name);
void mock_bluetooth_set_device_bda(const uint8_t *bda);

/* --- Configure return values for action functions --- */

void mock_bluetooth_set_enable_result(esp_err_t err);
void mock_bluetooth_set_disable_result(esp_err_t err);
void mock_bluetooth_set_scan_result(esp_err_t err);
void mock_bluetooth_set_connect_result(esp_err_t err);
void mock_bluetooth_set_disconnect_result(esp_err_t err);

/* --- Verify calls --- */

int mock_bluetooth_get_enable_count(void);
int mock_bluetooth_get_disable_count(void);
int mock_bluetooth_get_scan_count(void);
int mock_bluetooth_get_connect_count(void);
const uint8_t *mock_bluetooth_get_last_connect_bda(void);
int mock_bluetooth_get_disconnect_count(void);
