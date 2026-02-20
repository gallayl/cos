#include "unity.h"
#include "bluetooth_hid.h"

#include <string.h>

#define CAPTURE_BUF_SIZE 64

static uint8_t s_capture_buf[CAPTURE_BUF_SIZE];
static size_t s_capture_len;

static void test_keyboard_cb(const char *data, size_t len)
{
    for (size_t i = 0; i < len && s_capture_len < CAPTURE_BUF_SIZE; i++)
    {
        s_capture_buf[s_capture_len++] = (uint8_t)data[i];
    }
}

void setUp(void)
{
    s_capture_len = 0;
    memset(s_capture_buf, 0, sizeof(s_capture_buf));
    bluetooth_hid_set_keyboard_callback(test_keyboard_cb);
}

void tearDown(void) {}

/* --- Helper to build an 8-byte boot-protocol keyboard report --- */

static void make_report(uint8_t *report, uint8_t modifiers,
                        uint8_t k0, uint8_t k1, uint8_t k2,
                        uint8_t k3, uint8_t k4, uint8_t k5)
{
    report[0] = modifiers;
    report[1] = 0x00;
    report[2] = k0;
    report[3] = k1;
    report[4] = k2;
    report[5] = k3;
    report[6] = k4;
    report[7] = k5;
}

static void send_key(uint8_t modifiers, uint8_t keycode)
{
    uint8_t report[8];

    /* Key-down */
    make_report(report, modifiers, keycode, 0, 0, 0, 0, 0);
    bluetooth_hid_keyboard_input(report, sizeof(report));

    /* Key-up (all zeros) -- resets prev-key tracking */
    make_report(report, 0, 0, 0, 0, 0, 0, 0);
    bluetooth_hid_keyboard_input(report, sizeof(report));
}

/* --- Basic letter keys --- */

void test_key_a(void)
{
    send_key(0, 0x04);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('a', data[0]);
}

void test_key_z(void)
{
    send_key(0, 0x1D);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('z', data[0]);
}

/* --- Shifted letters --- */

void test_shift_a(void)
{
    send_key(0x02, 0x04); /* LShift + a */
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('A', data[0]);
}

void test_rshift_a(void)
{
    send_key(0x20, 0x04); /* RShift + a */
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('A', data[0]);
}

/* --- Digit keys --- */

void test_key_1(void)
{
    send_key(0, 0x1E);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('1', data[0]);
}

void test_key_0(void)
{
    send_key(0, 0x27);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('0', data[0]);
}

/* --- Shifted digit keys (symbols) --- */

void test_shift_1_is_exclamation(void)
{
    send_key(0x02, 0x1E);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('!', data[0]);
}

void test_shift_2_is_at(void)
{
    send_key(0x02, 0x1F);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('@', data[0]);
}

/* --- Special characters --- */

void test_enter(void)
{
    send_key(0, 0x28);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('\n', data[0]);
}

void test_space(void)
{
    send_key(0, 0x2C);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(' ', data[0]);
}

void test_backspace(void)
{
    send_key(0, 0x2A);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('\b', data[0]);
}

void test_tab(void)
{
    send_key(0, 0x2B);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('\t', data[0]);
}

void test_escape(void)
{
    send_key(0, 0x29);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(0x1B, data[0]);
}

/* --- Punctuation --- */

void test_minus(void)
{
    send_key(0, 0x2D);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('-', data[0]);
}

void test_shift_minus_is_underscore(void)
{
    send_key(0x02, 0x2D);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL('_', data[0]);
}

void test_semicolon(void)
{
    send_key(0, 0x33);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(';', data[0]);
}

void test_shift_semicolon_is_colon(void)
{
    send_key(0x02, 0x33);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(':', data[0]);
}

/* --- Ctrl combos --- */

void test_ctrl_a(void)
{
    send_key(0x01, 0x04); /* LCtrl + a → 0x01 (SOH) */
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(0x01, data[0]);
}

void test_ctrl_c(void)
{
    send_key(0x01, 0x06); /* LCtrl + c → 0x03 (ETX) */
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(0x03, data[0]);
}

void test_ctrl_z(void)
{
    send_key(0x01, 0x1D); /* LCtrl + z → 0x1A (SUB) */
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(0x1A, data[0]);
}

void test_rctrl_a(void)
{
    send_key(0x10, 0x04); /* RCtrl + a → 0x01 */
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(0x01, data[0]);
}

void test_ctrl_shift_a(void)
{
    send_key(0x01 | 0x02, 0x04); /* LCtrl + LShift + a → 0x01 (capital 'A' → ctrl) */
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(0x01, data[0]);
}

/* --- Arrow keys (VT100 sequences) --- */

void test_arrow_up(void)
{
    send_key(0, 0x52);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[A", data, 3);
}

void test_arrow_down(void)
{
    send_key(0, 0x51);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[B", data, 3);
}

void test_arrow_right(void)
{
    send_key(0, 0x4F);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[C", data, 3);
}

void test_arrow_left(void)
{
    send_key(0, 0x50);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[D", data, 3);
}

/* --- Navigation keys --- */

void test_delete(void)
{
    send_key(0, 0x4C);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[3~", data, 4);
}

void test_home(void)
{
    send_key(0, 0x4A);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[H", data, 3);
}

void test_end(void)
{
    send_key(0, 0x4D);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[F", data, 3);
}

void test_page_up(void)
{
    send_key(0, 0x4B);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[5~", data, 4);
}

void test_page_down(void)
{
    send_key(0, 0x4E);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[6~", data, 4);
}

void test_insert(void)
{
    send_key(0, 0x49);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[2~", data, 4);
}

/* --- Function keys --- */

void test_f1(void)
{
    send_key(0, 0x3A);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1bOP", data, 3);
}

void test_f4(void)
{
    send_key(0, 0x3D);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1bOS", data, 3);
}

void test_f5(void)
{
    send_key(0, 0x3E);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(5, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[15~", data, 5);
}

void test_f12(void)
{
    send_key(0, 0x45);
    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(5, len);
    TEST_ASSERT_EQUAL_MEMORY("\x1b[24~", data, 5);
}

/* --- Key repeat suppression --- */

void test_held_key_does_not_repeat(void)
{
    uint8_t report[8];

    make_report(report, 0, 0x04, 0, 0, 0, 0, 0);
    bluetooth_hid_keyboard_input(report, sizeof(report));

    /* Same report again (key still held) -- should not produce more output */
    s_capture_len = 0;
    bluetooth_hid_keyboard_input(report, sizeof(report));

    size_t len = s_capture_len;
    TEST_ASSERT_EQUAL(0, len);

    /* Release */
    make_report(report, 0, 0, 0, 0, 0, 0, 0);
    bluetooth_hid_keyboard_input(report, sizeof(report));
}

/* --- Multiple simultaneous keys --- */

void test_two_keys_at_once(void)
{
    uint8_t report[8];

    make_report(report, 0, 0x04, 0x05, 0, 0, 0, 0); /* a + b */
    bluetooth_hid_keyboard_input(report, sizeof(report));

    size_t len = s_capture_len;
    const uint8_t *data = s_capture_buf;
    TEST_ASSERT_EQUAL(2, len);
    TEST_ASSERT_EQUAL('a', data[0]);
    TEST_ASSERT_EQUAL('b', data[1]);

    /* Release */
    make_report(report, 0, 0, 0, 0, 0, 0, 0);
    bluetooth_hid_keyboard_input(report, sizeof(report));
}

/* --- Rollover error (phantom state) --- */

void test_rollover_error_ignored(void)
{
    uint8_t report[8];

    /* Rollover: all key slots = 0x01 */
    make_report(report, 0, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01);
    bluetooth_hid_keyboard_input(report, sizeof(report));

    size_t len = s_capture_len;
    TEST_ASSERT_EQUAL(0, len);

    /* Release */
    make_report(report, 0, 0, 0, 0, 0, 0, 0);
    bluetooth_hid_keyboard_input(report, sizeof(report));
}

/* --- NULL / short report guard --- */

void test_null_report_ignored(void)
{
    bluetooth_hid_keyboard_input(NULL, 8);
    size_t len = s_capture_len;
    TEST_ASSERT_EQUAL(0, len);
}

void test_short_report_ignored(void)
{
    uint8_t report[4] = {0};
    bluetooth_hid_keyboard_input(report, sizeof(report));
    size_t len = s_capture_len;
    TEST_ASSERT_EQUAL(0, len);
}

/* --- Unknown keycode produces no output --- */

void test_unknown_keycode_ignored(void)
{
    send_key(0, 0x7F); /* beyond lookup table */
    size_t len = s_capture_len;
    TEST_ASSERT_EQUAL(0, len);
}

void test_keycode_zero_ignored(void)
{
    uint8_t report[8];
    make_report(report, 0, 0x00, 0, 0, 0, 0, 0);
    bluetooth_hid_keyboard_input(report, sizeof(report));
    size_t len = s_capture_len;
    TEST_ASSERT_EQUAL(0, len);
}

/* --- main --- */

int main(void)
{
    UNITY_BEGIN();

    /* Letters */
    RUN_TEST(test_key_a);
    RUN_TEST(test_key_z);

    /* Shifted letters */
    RUN_TEST(test_shift_a);
    RUN_TEST(test_rshift_a);

    /* Digits */
    RUN_TEST(test_key_1);
    RUN_TEST(test_key_0);

    /* Shifted digits (symbols) */
    RUN_TEST(test_shift_1_is_exclamation);
    RUN_TEST(test_shift_2_is_at);

    /* Special characters */
    RUN_TEST(test_enter);
    RUN_TEST(test_space);
    RUN_TEST(test_backspace);
    RUN_TEST(test_tab);
    RUN_TEST(test_escape);

    /* Punctuation */
    RUN_TEST(test_minus);
    RUN_TEST(test_shift_minus_is_underscore);
    RUN_TEST(test_semicolon);
    RUN_TEST(test_shift_semicolon_is_colon);

    /* Ctrl combos */
    RUN_TEST(test_ctrl_a);
    RUN_TEST(test_ctrl_c);
    RUN_TEST(test_ctrl_z);
    RUN_TEST(test_rctrl_a);
    RUN_TEST(test_ctrl_shift_a);

    /* Arrow keys */
    RUN_TEST(test_arrow_up);
    RUN_TEST(test_arrow_down);
    RUN_TEST(test_arrow_right);
    RUN_TEST(test_arrow_left);

    /* Navigation keys */
    RUN_TEST(test_delete);
    RUN_TEST(test_home);
    RUN_TEST(test_end);
    RUN_TEST(test_page_up);
    RUN_TEST(test_page_down);
    RUN_TEST(test_insert);

    /* Function keys */
    RUN_TEST(test_f1);
    RUN_TEST(test_f4);
    RUN_TEST(test_f5);
    RUN_TEST(test_f12);

    /* Repeat suppression */
    RUN_TEST(test_held_key_does_not_repeat);

    /* Multiple keys */
    RUN_TEST(test_two_keys_at_once);

    /* Edge cases */
    RUN_TEST(test_rollover_error_ignored);
    RUN_TEST(test_null_report_ignored);
    RUN_TEST(test_short_report_ignored);
    RUN_TEST(test_unknown_keycode_ignored);
    RUN_TEST(test_keycode_zero_ignored);

    return UNITY_END();
}
