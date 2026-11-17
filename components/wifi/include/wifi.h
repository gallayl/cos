#pragma once

#include "esp_err.h"
#include "esp_wifi_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Managed Wi-Fi lifecycle state (radio/AP/STA activity), not STA link/IP state. */
    typedef enum
    {
        WIFI_SERVICE_STOPPED = 0,
        WIFI_SERVICE_STARTED,
    } wifi_service_state_t;

    /**
     * Initialize the WiFi subsystem (netifs/driver/handlers/command registration) and
     * apply boot-time effective state before any radio/AP/STA bring-up.
     *
     * Boot precedence: if the active boot profile forces Wi-Fi off (e.g. low-memory-offline),
     * Wi-Fi remains off regardless of persisted enabled state. Otherwise persisted enabled state
     * is honored (default enabled). No unintended AP/STA/radio bring-up occurs when forced off
     * or persisted disabled. Registers shell commands.
     */
    esp_err_t wifi_init(void);

    /**
     * Tear down the WiFi subsystem: stop radio/service best-effort and release lifecycle resources.
     * After this call, wifi_init() may be called again.
     */
    esp_err_t wifi_shutdown(void);

    /**
     * Start/enable Wi-Fi service (radio/AP/STA as applicable) and persist enabled state.
     * Fails with ESP_ERR_NOT_ALLOWED if forced off by boot profile for this boot.
     * No-op success if already started.
     */
    esp_err_t wifi_start(void);

    /**
     * Stop/disable Wi-Fi service (radio down/disconnect) with best-effort cleanup of safely
     * reclaimable runtime resources and persist disabled state. No-op success if already stopped.
     * Does not perform full esp_wifi_deinit/netif/event-loop teardown.
     */
    esp_err_t wifi_stop(void);

    /**
     * Restart active Wi-Fi service (reconnect/restart cycle) while preserving persisted enabled
     * intent. Allowed only when Wi-Fi service is active/started.
     * Returns ESP_ERR_INVALID_STATE when stopped/disabled/forced-off (no auto-start).
     */
    esp_err_t wifi_restart(void);

    /** Current managed service state. */
    wifi_service_state_t wifi_get_service_state(void);

    /** True when service is started/active. */
    bool wifi_is_started(void);

    /** True when Wi-Fi is forced off by active boot profile for this boot. */
    bool wifi_is_forced_off_by_profile(void);

    /** Persisted user-enabled intent (subject to boot-profile precedence at boot/runtime). */
    bool wifi_is_user_enabled(void);

    /**
     * Connect to a WiFi network. Saves credentials to NVS (via esp_wifi)
     * and begins connection. Returns immediately; check wifi_is_connected()
     * or monitor logs for connection result.
     * Requires Wi-Fi service to be started; returns ESP_ERR_INVALID_STATE otherwise.
     */
    esp_err_t wifi_connect(const char *ssid, const char *password);

    /**
     * Disconnect from the current WiFi network.
     * Requires Wi-Fi service to be started; returns ESP_ERR_INVALID_STATE otherwise.
     */
    esp_err_t wifi_disconnect(void);

    /**
     * Scan for available WiFi networks (blocking scan).
     * @param results   Caller-provided buffer for scan results.
     * @param count     On return, number of APs found (up to max_results).
     * @param max_results  Maximum entries to store in results[].
     * Requires Wi-Fi service to be started; returns ESP_ERR_INVALID_STATE otherwise.
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

    /** Test-only helper: reset internal component state (no hardware side effects). */
    void wifi_test_reset(void);

#ifdef __cplusplus
}
#endif
