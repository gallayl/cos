#include "unity.h"
#include "i2c_bus.h"
#include "mock_i2c.h"
#include "mock_console.h"

void setUp(void)
{
    mock_i2c_reset();
    mock_console_reset();
}

void tearDown(void)
{
}

/* --- i2c_bus_scan --- */

void test_scan_finds_devices(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_init());

    uint8_t probed[] = {0x3C, 0x50, 0x68};
    mock_i2c_set_probe_devices(probed, 3);

    uint8_t addrs[I2C_BUS_MAX_DEVICES];
    size_t found = 0;
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_scan(addrs, I2C_BUS_MAX_DEVICES, &found));
    TEST_ASSERT_EQUAL(3, found);
    TEST_ASSERT_EQUAL_UINT8(0x3C, addrs[0]);
    TEST_ASSERT_EQUAL_UINT8(0x50, addrs[1]);
    TEST_ASSERT_EQUAL_UINT8(0x68, addrs[2]);
}

void test_scan_no_devices(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_init());

    uint8_t addrs[I2C_BUS_MAX_DEVICES];
    size_t found = 0;
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_scan(addrs, I2C_BUS_MAX_DEVICES, &found));
    TEST_ASSERT_EQUAL(0, found);
}

void test_scan_null_args(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_init());

    size_t found = 0;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, i2c_bus_scan(NULL, 10, &found));

    uint8_t addrs[10];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, i2c_bus_scan(addrs, 10, NULL));
}

void test_scan_found_exceeds_buffer(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_init());

    uint8_t probed[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    mock_i2c_set_probe_devices(probed, 5);

    uint8_t addrs[2];
    size_t found = 0;
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_scan(addrs, 2, &found));
    /* found reports total count even when buffer is smaller */
    TEST_ASSERT_EQUAL(5, found);
    TEST_ASSERT_EQUAL_UINT8(0x10, addrs[0]);
    TEST_ASSERT_EQUAL_UINT8(0x20, addrs[1]);
}

/* --- i2c_bus_read --- */

void test_read_receives_data(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_init());

    uint8_t rx[] = {0xAA, 0xBB, 0xCC};
    mock_i2c_set_rx_data(rx, 3);

    uint8_t buf[3] = {0};
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_read(0x50, buf, 3));
    TEST_ASSERT_EQUAL_UINT8(0xAA, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, buf[1]);
    TEST_ASSERT_EQUAL_UINT8(0xCC, buf[2]);
    TEST_ASSERT_EQUAL_UINT16(0x50, mock_i2c_get_last_device_addr());
}

void test_read_null_args(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_init());
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, i2c_bus_read(0x50, NULL, 4));

    uint8_t buf[4];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, i2c_bus_read(0x50, buf, 0));
}

void test_read_propagates_error(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_init());
    mock_i2c_set_next_error(ESP_FAIL);

    uint8_t buf[4];
    TEST_ASSERT_EQUAL(ESP_FAIL, i2c_bus_read(0x50, buf, 4));
}

/* --- i2c_bus_write --- */

void test_write_sends_data(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_init());

    uint8_t data[] = {0x01, 0x02, 0x03};
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_write(0x3C, data, 3));

    size_t tx_len = 0;
    const uint8_t *tx = mock_i2c_get_tx_data(&tx_len);
    TEST_ASSERT_EQUAL(3, tx_len);
    TEST_ASSERT_EQUAL_UINT8(0x01, tx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, tx[1]);
    TEST_ASSERT_EQUAL_UINT8(0x03, tx[2]);
    TEST_ASSERT_EQUAL_UINT16(0x3C, mock_i2c_get_last_device_addr());
}

void test_write_null_args(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_init());
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, i2c_bus_write(0x3C, NULL, 4));

    uint8_t data[] = {0x01};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, i2c_bus_write(0x3C, data, 0));
}

void test_write_propagates_error(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, i2c_bus_init());
    mock_i2c_set_next_error(ESP_FAIL);

    uint8_t data[] = {0x01};
    TEST_ASSERT_EQUAL(ESP_FAIL, i2c_bus_write(0x3C, data, 1));
}

/* --- i2c_bus_init error --- */

void test_init_propagates_bus_error(void)
{
    mock_i2c_set_next_error(ESP_FAIL);
    TEST_ASSERT_EQUAL(ESP_FAIL, i2c_bus_init());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_scan_finds_devices);
    RUN_TEST(test_scan_no_devices);
    RUN_TEST(test_scan_null_args);
    RUN_TEST(test_scan_found_exceeds_buffer);

    RUN_TEST(test_read_receives_data);
    RUN_TEST(test_read_null_args);
    RUN_TEST(test_read_propagates_error);

    RUN_TEST(test_write_sends_data);
    RUN_TEST(test_write_null_args);
    RUN_TEST(test_write_propagates_error);

    RUN_TEST(test_init_propagates_bus_error);

    return UNITY_END();
}
