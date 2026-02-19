#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize the Bluetooth component: register shell commands.
     * BT stack is NOT started -- call bluetooth_enable() (or `bt on`) to start it.
     */
    esp_err_t bluetooth_init(void);

    /**
     * Start the BT controller, Bluedroid stack, and HID host.
     * Enables GAP discovery and SSP pairing support.
     */
    esp_err_t bluetooth_enable(void);

    /**
     * Stop the BT stack: disconnect any device, deinit HID host,
     * disable and deinit Bluedroid, then disable and deinit the controller.
     */
    esp_err_t bluetooth_disable(void);

    /**
     * Start Classic BT GAP discovery for nearby devices.
     * Results are printed to stdout as they arrive; discovery runs for ~10 seconds.
     */
    esp_err_t bluetooth_scan(void);

    /**
     * Connect to a BT HID device by its BD address.
     * Pairing (SSP) is handled automatically.
     * @param bda  6-byte Bluetooth device address
     */
    esp_err_t bluetooth_connect(const uint8_t *bda);

    /** Disconnect the currently connected HID device. */
    esp_err_t bluetooth_disconnect(void);

    /** True if the BT stack is enabled and running. */
    bool bluetooth_is_enabled(void);

    /** True if bluetooth_enable() has been called and the stack is still starting. */
    bool bluetooth_is_enabling(void);

    /** True if HID host is initialized (required for connect; may lag a few seconds after enable). */
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
