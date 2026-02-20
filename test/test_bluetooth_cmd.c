#include "unity.h"
#include "bluetooth.h"
#include "mock_bluetooth.h"
#include "mock_console.h"

void bluetooth_register_commands(void);

void setUp(void)
{
    mock_bluetooth_reset();
    mock_console_reset();
    bluetooth_register_commands();
}

void tearDown(void) {}

/* --- Command registration --- */

void test_bt_command_registered(void)
{
    char *argv[] = {"bt"};
    int ret = mock_console_run_cmd("bt", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

/* --- bt (no args) shows status --- */

void test_bt_status_shows_off(void)
{
    char *argv[] = {"bt"};
    int ret = mock_console_run_cmd("bt", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_bt_status_shows_on(void)
{
    mock_bluetooth_set_enabled(true);
    char *argv[] = {"bt"};
    int ret = mock_console_run_cmd("bt", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_bt_status_shows_enabling(void)
{
    mock_bluetooth_set_enabling(true);
    char *argv[] = {"bt"};
    int ret = mock_console_run_cmd("bt", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_bt_status_shows_connected_device(void)
{
    mock_bluetooth_set_enabled(true);
    mock_bluetooth_set_connected(true);
    mock_bluetooth_set_device_name("TestKeyboard");
    uint8_t bda[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    mock_bluetooth_set_device_bda(bda);

    char *argv[] = {"bt"};
    int ret = mock_console_run_cmd("bt", 1, argv);
    TEST_ASSERT_EQUAL(0, ret);
}

/* --- bt on --- */

void test_bt_on(void)
{
    char *argv[] = {"bt", "on"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(1, mock_bluetooth_get_enable_count());
}

void test_bt_on_failure(void)
{
    mock_bluetooth_set_enable_result(ESP_ERR_INVALID_STATE);
    char *argv[] = {"bt", "on"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

/* --- bt off --- */

void test_bt_off(void)
{
    char *argv[] = {"bt", "off"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(1, mock_bluetooth_get_disable_count());
}

void test_bt_off_failure(void)
{
    mock_bluetooth_set_disable_result(ESP_ERR_INVALID_STATE);
    char *argv[] = {"bt", "off"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

/* --- bt scan --- */

void test_bt_scan_when_enabled(void)
{
    mock_bluetooth_set_enabled(true);
    char *argv[] = {"bt", "scan"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(1, mock_bluetooth_get_scan_count());
}

void test_bt_scan_when_disabled(void)
{
    char *argv[] = {"bt", "scan"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(0, mock_bluetooth_get_scan_count());
}

/* --- bt connect --- */

void test_bt_connect_valid_address(void)
{
    mock_bluetooth_set_enabled(true);
    mock_bluetooth_set_hid_ready(true);
    char *argv[] = {"bt", "connect", "aa:bb:cc:dd:ee:ff"};
    int ret = mock_console_run_cmd("bt", 3, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(1, mock_bluetooth_get_connect_count());

    const uint8_t expected_bda[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    TEST_ASSERT_EQUAL_MEMORY(expected_bda, mock_bluetooth_get_last_connect_bda(), 6);
}

void test_bt_connect_uppercase_address(void)
{
    mock_bluetooth_set_enabled(true);
    mock_bluetooth_set_hid_ready(true);
    char *argv[] = {"bt", "connect", "AA:BB:CC:DD:EE:FF"};
    int ret = mock_console_run_cmd("bt", 3, argv);
    TEST_ASSERT_EQUAL(0, ret);

    const uint8_t expected_bda[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    TEST_ASSERT_EQUAL_MEMORY(expected_bda, mock_bluetooth_get_last_connect_bda(), 6);
}

void test_bt_connect_invalid_address(void)
{
    mock_bluetooth_set_enabled(true);
    mock_bluetooth_set_hid_ready(true);
    char *argv[] = {"bt", "connect", "not-an-address"};
    int ret = mock_console_run_cmd("bt", 3, argv);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(0, mock_bluetooth_get_connect_count());
}

void test_bt_connect_missing_address(void)
{
    mock_bluetooth_set_enabled(true);
    mock_bluetooth_set_hid_ready(true);
    char *argv[] = {"bt", "connect"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(0, mock_bluetooth_get_connect_count());
}

void test_bt_connect_when_disabled(void)
{
    char *argv[] = {"bt", "connect", "aa:bb:cc:dd:ee:ff"};
    int ret = mock_console_run_cmd("bt", 3, argv);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(0, mock_bluetooth_get_connect_count());
}

void test_bt_connect_when_hid_not_ready(void)
{
    mock_bluetooth_set_enabled(true);
    /* hid_ready = false (default) */
    char *argv[] = {"bt", "connect", "aa:bb:cc:dd:ee:ff"};
    int ret = mock_console_run_cmd("bt", 3, argv);
    TEST_ASSERT_EQUAL(1, ret);
    TEST_ASSERT_EQUAL(0, mock_bluetooth_get_connect_count());
}

void test_bt_connect_failure(void)
{
    mock_bluetooth_set_enabled(true);
    mock_bluetooth_set_hid_ready(true);
    mock_bluetooth_set_connect_result(ESP_FAIL);
    char *argv[] = {"bt", "connect", "aa:bb:cc:dd:ee:ff"};
    int ret = mock_console_run_cmd("bt", 3, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

/* --- bt disconnect --- */

void test_bt_disconnect(void)
{
    char *argv[] = {"bt", "disconnect"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(1, mock_bluetooth_get_disconnect_count());
}

void test_bt_disconnect_failure(void)
{
    mock_bluetooth_set_disconnect_result(ESP_ERR_INVALID_STATE);
    char *argv[] = {"bt", "disconnect"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

/* --- bt forget --- */

void test_bt_forget(void)
{
    char *argv[] = {"bt", "forget"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(1, mock_bluetooth_get_forget_count());
}

void test_bt_forget_failure(void)
{
    mock_bluetooth_set_forget_result(ESP_FAIL);
    char *argv[] = {"bt", "forget"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

/* --- Unknown subcommand --- */

void test_bt_unknown_subcommand(void)
{
    char *argv[] = {"bt", "bogus"};
    int ret = mock_console_run_cmd("bt", 2, argv);
    TEST_ASSERT_EQUAL(1, ret);
}

/* --- main --- */

int main(void)
{
    UNITY_BEGIN();

    /* Command registration */
    RUN_TEST(test_bt_command_registered);

    /* Status display */
    RUN_TEST(test_bt_status_shows_off);
    RUN_TEST(test_bt_status_shows_on);
    RUN_TEST(test_bt_status_shows_enabling);
    RUN_TEST(test_bt_status_shows_connected_device);

    /* Enable/disable */
    RUN_TEST(test_bt_on);
    RUN_TEST(test_bt_on_failure);
    RUN_TEST(test_bt_off);
    RUN_TEST(test_bt_off_failure);

    /* Scan */
    RUN_TEST(test_bt_scan_when_enabled);
    RUN_TEST(test_bt_scan_when_disabled);

    /* Connect */
    RUN_TEST(test_bt_connect_valid_address);
    RUN_TEST(test_bt_connect_uppercase_address);
    RUN_TEST(test_bt_connect_invalid_address);
    RUN_TEST(test_bt_connect_missing_address);
    RUN_TEST(test_bt_connect_when_disabled);
    RUN_TEST(test_bt_connect_when_hid_not_ready);
    RUN_TEST(test_bt_connect_failure);

    /* Disconnect */
    RUN_TEST(test_bt_disconnect);
    RUN_TEST(test_bt_disconnect_failure);

    /* Forget */
    RUN_TEST(test_bt_forget);
    RUN_TEST(test_bt_forget_failure);

    /* Unknown */
    RUN_TEST(test_bt_unknown_subcommand);

    return UNITY_END();
}
