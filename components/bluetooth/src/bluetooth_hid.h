#pragma once

#include "bluetooth.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Process a raw HID keyboard input report and forward the resulting
     * characters to the registered callback.
     *
     * @param data   Pointer to the HID report bytes
     * @param len    Length of the report in bytes
     */
    void bluetooth_hid_keyboard_input(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
