#include "bluetooth_hid.h"

#include <stdbool.h>
#include <string.h>

/* Standard 8-byte boot-protocol keyboard report:
 *   [0] modifier mask  (bit0=LCtrl, 1=LShift, 2=LAlt, 3=LGui,
 *                        4=RCtrl, 5=RShift, 6=RAlt, 7=RGui)
 *   [1] reserved (0x00)
 *   [2..7] up to 6 simultaneous keycodes
 */

#define MOD_LCTRL (1 << 0)
#define MOD_LSHIFT (1 << 1)
#define MOD_LALT (1 << 2)
#define MOD_LGUI (1 << 3)
#define MOD_RCTRL (1 << 4)
#define MOD_RSHIFT (1 << 5)
#define MOD_RALT (1 << 6)
#define MOD_RGUI (1 << 7)

#define MOD_SHIFT (MOD_LSHIFT | MOD_RSHIFT)
#define MOD_CTRL (MOD_LCTRL | MOD_RCTRL)

#define MAX_KEYS 6

static volatile bt_keyboard_cb_t s_keyboard_cb;
static volatile bt_keyboard_event_cb_t s_key_event_cb;

void bluetooth_hid_set_keyboard_callback(bt_keyboard_cb_t cb)
{
    s_keyboard_cb = cb;
}

void bluetooth_hid_set_key_event_callback(bt_keyboard_event_cb_t cb)
{
    s_key_event_cb = cb;
}

/* USB HID keycode → ASCII (US layout, lowercase) */
static const char KEYCODE_TO_ASCII[128] = {
    [0x00] = 0, /* No event */
    [0x04] = 'a',  [0x05] = 'b', [0x06] = 'c', [0x07] = 'd', [0x08] = 'e',  [0x09] = 'f', [0x0A] = 'g',  [0x0B] = 'h',
    [0x0C] = 'i',  [0x0D] = 'j', [0x0E] = 'k', [0x0F] = 'l', [0x10] = 'm',  [0x11] = 'n', [0x12] = 'o',  [0x13] = 'p',
    [0x14] = 'q',  [0x15] = 'r', [0x16] = 's', [0x17] = 't', [0x18] = 'u',  [0x19] = 'v', [0x1A] = 'w',  [0x1B] = 'x',
    [0x1C] = 'y',  [0x1D] = 'z', [0x1E] = '1', [0x1F] = '2', [0x20] = '3',  [0x21] = '4', [0x22] = '5',  [0x23] = '6',
    [0x24] = '7',  [0x25] = '8', [0x26] = '9', [0x27] = '0', [0x28] = '\n', /* Enter */
    [0x29] = 0x1B,                                                          /* Escape */
    [0x2A] = '\b',                                                          /* Backspace */
    [0x2B] = '\t',                                                          /* Tab */
    [0x2C] = ' ',                                                           /* Space */
    [0x2D] = '-',  [0x2E] = '=', [0x2F] = '[', [0x30] = ']', [0x31] = '\\', [0x33] = ';', [0x34] = '\'', [0x35] = '`',
    [0x36] = ',',  [0x37] = '.', [0x38] = '/',
};

/* Shifted versions of the above */
static const char KEYCODE_TO_ASCII_SHIFT[128] = {
    [0x04] = 'A', [0x05] = 'B', [0x06] = 'C', [0x07] = 'D', [0x08] = 'E',  [0x09] = 'F',  [0x0A] = 'G',  [0x0B] = 'H',
    [0x0C] = 'I', [0x0D] = 'J', [0x0E] = 'K', [0x0F] = 'L', [0x10] = 'M',  [0x11] = 'N',  [0x12] = 'O',  [0x13] = 'P',
    [0x14] = 'Q', [0x15] = 'R', [0x16] = 'S', [0x17] = 'T', [0x18] = 'U',  [0x19] = 'V',  [0x1A] = 'W',  [0x1B] = 'X',
    [0x1C] = 'Y', [0x1D] = 'Z', [0x1E] = '!', [0x1F] = '@', [0x20] = '#',  [0x21] = '$',  [0x22] = '%',  [0x23] = '^',
    [0x24] = '&', [0x25] = '*', [0x26] = '(', [0x27] = ')', [0x28] = '\n', [0x29] = 0x1B, [0x2A] = '\b', [0x2B] = '\t',
    [0x2C] = ' ', [0x2D] = '_', [0x2E] = '+', [0x2F] = '{', [0x30] = '}',  [0x31] = '|',  [0x33] = ':',  [0x34] = '"',
    [0x35] = '~', [0x36] = '<', [0x37] = '>', [0x38] = '?',
};

/* Track previous report to detect newly pressed keys */
static uint8_t s_prev_keys[MAX_KEYS];

static char keycode_to_char(uint8_t keycode, uint8_t modifiers)
{
    if (keycode == 0 || keycode >= 128)
    {
        return 0;
    }

    char ch;
    if (modifiers & MOD_SHIFT)
    {
        ch = KEYCODE_TO_ASCII_SHIFT[keycode];
    }
    else
    {
        ch = KEYCODE_TO_ASCII[keycode];
    }

    if (ch != 0 && (modifiers & MOD_CTRL))
    {
        /* Ctrl+A..Z → 0x01..0x1A */
        if (ch >= 'a' && ch <= 'z')
        {
            ch = (char)(ch - 'a' + 1);
        }
        else if (ch >= 'A' && ch <= 'Z')
        {
            ch = (char)(ch - 'A' + 1);
        }
    }

    return ch;
}

static bool key_in_prev(uint8_t keycode)
{
    for (int i = 0; i < MAX_KEYS; i++)
    {
        if (s_prev_keys[i] == keycode)
        {
            return true;
        }
    }
    return false;
}

static bool key_in_current(const uint8_t *keys, uint8_t keycode)
{
    for (int i = 0; i < MAX_KEYS; i++)
    {
        if (keys[i] == keycode)
        {
            return true;
        }
    }
    return false;
}

static const char *keycode_to_sequence(uint8_t keycode, size_t *len)
{
    switch (keycode)
    {
    case 0x3A:
        *len = 3;
        return "\x1bOP"; /* F1 */
    case 0x3B:
        *len = 3;
        return "\x1bOQ"; /* F2 */
    case 0x3C:
        *len = 3;
        return "\x1bOR"; /* F3 */
    case 0x3D:
        *len = 3;
        return "\x1bOS"; /* F4 */
    case 0x3E:
        *len = 5;
        return "\x1b[15~"; /* F5 */
    case 0x3F:
        *len = 5;
        return "\x1b[17~"; /* F6 */
    case 0x40:
        *len = 5;
        return "\x1b[18~"; /* F7 */
    case 0x41:
        *len = 5;
        return "\x1b[19~"; /* F8 */
    case 0x42:
        *len = 5;
        return "\x1b[20~"; /* F9 */
    case 0x43:
        *len = 5;
        return "\x1b[21~"; /* F10 */
    case 0x44:
        *len = 5;
        return "\x1b[23~"; /* F11 */
    case 0x45:
        *len = 5;
        return "\x1b[24~"; /* F12 */
    case 0x49:
        *len = 4;
        return "\x1b[2~"; /* Insert */
    case 0x4A:
        *len = 3;
        return "\x1b[H"; /* Home */
    case 0x4B:
        *len = 4;
        return "\x1b[5~"; /* Page Up */
    case 0x4C:
        *len = 4;
        return "\x1b[3~"; /* Delete */
    case 0x4D:
        *len = 3;
        return "\x1b[F"; /* End */
    case 0x4E:
        *len = 4;
        return "\x1b[6~"; /* Page Down */
    case 0x4F:
        *len = 3;
        return "\x1b[C"; /* Right */
    case 0x50:
        *len = 3;
        return "\x1b[D"; /* Left */
    case 0x51:
        *len = 3;
        return "\x1b[B"; /* Down */
    case 0x52:
        *len = 3;
        return "\x1b[A"; /* Up */
    default:
        *len = 0;
        return NULL;
    }
}

void bluetooth_hid_keyboard_input(const uint8_t *data, size_t len)
{
    if (data == NULL || len < 8)
    {
        return;
    }

    uint8_t modifiers = data[0];
    const uint8_t *keys = &data[2];

    /* Rollover error: all keys are 0x01 (phantom state) */
    if (keys[0] == 0x01)
    {
        return;
    }

    char buf[MAX_KEYS];
    int buf_len = 0;

    for (int i = 0; i < MAX_KEYS; i++)
    {
        uint8_t kc = keys[i];
        if (kc == 0 || key_in_prev(kc))
        {
            continue;
        }

        keyboard_event_t event = {
            .keycode = kc,
            .modifiers = modifiers,
            .pressed = true,
        };
        size_t seq_len = 0;
        const char *seq = keycode_to_sequence(kc, &seq_len);
        if (seq != NULL)
        {
            memcpy(event.text, seq, seq_len);
            event.text_len = seq_len;
        }
        else
        {
            const char ch = keycode_to_char(kc, modifiers);
            if (ch != 0)
            {
                event.text[0] = ch;
                event.text_len = 1;
            }
        }

        if (s_key_event_cb != NULL)
        {
            s_key_event_cb(&event);
        }
        if (seq != NULL && s_keyboard_cb != NULL)
        {
            s_keyboard_cb(event.text, event.text_len);
        }
        if (seq == NULL && event.text_len > 0)
        {
            buf[buf_len++] = event.text[0];
        }
    }

    if (buf_len > 0 && s_keyboard_cb != NULL)
    {
        s_keyboard_cb(buf, buf_len);
    }

    for (int i = 0; i < MAX_KEYS; i++)
    {
        const uint8_t kc = s_prev_keys[i];
        if (kc == 0 || key_in_current(keys, kc))
        {
            continue;
        }

        const keyboard_event_t event = {
            .keycode = kc,
            .modifiers = modifiers,
            .pressed = false,
        };
        if (s_key_event_cb != NULL)
        {
            s_key_event_cb(&event);
        }
    }

    memcpy(s_prev_keys, keys, MAX_KEYS);
}

void bluetooth_hid_keyboard_reset(void)
{
    for (int i = 0; i < MAX_KEYS; i++)
    {
        const uint8_t keycode = s_prev_keys[i];
        if (keycode == 0)
        {
            continue;
        }

        keyboard_event_t event = {
            .keycode = keycode,
            .pressed = false,
        };
        if (s_key_event_cb != NULL)
        {
            s_key_event_cb(&event);
        }
    }
    memset(s_prev_keys, 0, sizeof(s_prev_keys));
}
