#pragma once

#include "esp_wifi_types.h"

#include <stdint.h>

/** Reset all mock WiFi state. Call before each test. */
void mock_wifi_reset(void);

/** Pre-load stored STA credentials (simulates NVS-persisted config). */
void mock_wifi_set_stored_sta_config(const char *ssid, const char *password);

/** Set scan results that will be returned by the next wifi_scan(). */
void mock_wifi_set_scan_results(const wifi_ap_record_t *records, uint16_t count);

/** Set the AP info returned by esp_wifi_sta_get_ap_info(). */
void mock_wifi_set_ap_info(const wifi_ap_record_t *info);

/** Get the current mock WiFi mode. */
wifi_mode_t mock_wifi_get_mode(void);

/** Get the last STA config set via esp_wifi_set_config(WIFI_IF_STA, ...). */
const wifi_config_t *mock_wifi_get_sta_config(void);

/** Get the last AP config set via esp_wifi_set_config(WIFI_IF_AP, ...). */
const wifi_config_t *mock_wifi_get_ap_config(void);

/** Get the connect call count. */
int mock_wifi_get_connect_count(void);

/** Get the disconnect call count. */
int mock_wifi_get_disconnect_count(void);
