#pragma once

#include "esp_err.h"
#include "esp_wifi_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize the WiFi subsystem: create netifs, register event handlers,
     * start AP mode, and auto-connect STA if stored credentials exist.
     * Registers shell commands. Non-blocking (does not wait for IP).
     */
    esp_err_t wifi_init(void);

    /**
     * Tear down the WiFi subsystem: disconnect, stop, and release resources.
     * After this call, wifi_init() may be called again.
     */
    esp_err_t wifi_deinit(void);

    /**
     * Connect to a WiFi network. Saves credentials to NVS (via esp_wifi)
     * and begins connection. Returns immediately; check wifi_is_connected()
     * or monitor logs for connection result.
     */
    esp_err_t wifi_connect(const char *ssid, const char *password);

    /** Disconnect from the current WiFi network. */
    esp_err_t wifi_disconnect(void);

    /**
     * Scan for available WiFi networks (blocking scan).
     * @param results   Caller-provided buffer for scan results.
     * @param count     On return, number of APs found (up to max_results).
     * @param max_results  Maximum entries to store in results[].
     */
    esp_err_t wifi_scan(wifi_ap_record_t *results, uint16_t *count, uint16_t max_results);

    /** True if STA has obtained an IP address. */
    bool wifi_is_connected(void);

    /** True if NVS contains stored STA credentials (non-empty SSID). */
    bool wifi_has_stored_credentials(void);

    /** Human-readable signal strength label for a given RSSI value. */
    const char *wifi_signal_strength_str(int8_t rssi);

    /** Human-readable name for a wifi_auth_mode_t value. */
    const char *wifi_auth_mode_str(wifi_auth_mode_t mode);

#ifdef __cplusplus
}
#endif
