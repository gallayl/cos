#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Normalized key transition. keycode is the USB HID usage ID; text is the
     * US-layout terminal representation on key-down only. Held reports do not
     * generate repeats; consumers can use the pressed state for continuous
     * controls.
     */
    typedef struct
    {
        uint8_t keycode;   /**< USB HID usage ID. */
        uint8_t modifiers; /**< HID modifier bitmask. */
        bool pressed;      /**< True for a press transition, false for release. */
        char text[5];      /**< Translated terminal bytes on key-down, if any. */
        size_t text_len;   /**< Number of valid bytes in text. */
    } keyboard_event_t;

#ifdef __cplusplus
}
#endif
