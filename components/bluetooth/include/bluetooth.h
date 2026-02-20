#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Callback invoked with translated keyboard characters (ASCII or VT100
     * escape sequences).  The data is NOT null-terminated.
     */
    typedef void (*bt_keyboard_cb_t)(const char *data, size_t len);

    /**
     * Register a callback to receive translated keyboard input from a
     * connected BLE HID keyboard.  Pass NULL to clear.
     */
    void bluetooth_hid_set_keyboard_callback(bt_keyboard_cb_t cb);

    /**
     * @brief Initialize the Bluetooth component: register shell commands.
     *
     * BT stack is NOT started -- call bluetooth_enable() (or `bt on`).
     */
    esp_err_t bluetooth_init(void);

    /**
     * @brief Start the BLE controller, Bluedroid stack, and HID host.
     *
     * Configures BLE security and GATT client for HID over GATT (HOGP).
     */
    esp_err_t bluetooth_enable(void);

    /**
     * @brief Stop the BLE stack: disconnect any device, deinit HID host,
     * disable and deinit Bluedroid, then disable and deinit the controller.
     */
    esp_err_t bluetooth_disable(void);

    /**
     * @brief Start BLE GAP scan for nearby HID devices.
     *
     * Only devices advertising the HID Service UUID (0x1812) are shown.
     * Scan runs for ~10 seconds.
     */
    esp_err_t bluetooth_scan(void);

    /**
     * @brief Connect to a BLE HID device by its BD address.
     *
     * The address type is looked up from the last scan results.
     * BLE pairing is handled automatically ("Just Works").
     *
     * @param bda  6-byte Bluetooth device address
     */
    esp_err_t bluetooth_connect(const uint8_t *bda);

    /** Disconnect the currently connected HID device. */
    esp_err_t bluetooth_disconnect(void);

    /**
     * @brief Clear the saved auto-connect device and remove all BLE bonding keys.
     *
     * Disconnects the current device first if connected.
     */
    esp_err_t bluetooth_forget(void);

    /** True if the BLE stack is enabled and running. */
    bool bluetooth_is_enabled(void);

    /** True if bluetooth_enable() has been called and the stack is still starting. */
    bool bluetooth_is_enabling(void);

    /** True if HID host is initialized (required for connect). */
    bool bluetooth_is_hid_ready(void);

    /** True if a HID device is currently connected. */
    bool bluetooth_is_connected(void);

    /** Name of the connected HID device, or NULL if none. */
    const char *bluetooth_connected_device_name(void);

    /** BD address of the connected HID device, or NULL if none. */
    const uint8_t *bluetooth_connected_device_bda(void);

#ifdef __cplusplus
}
#endif
