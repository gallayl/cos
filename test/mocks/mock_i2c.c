#include "mock_i2c.h"
#include "driver/i2c_master.h"

#include <string.h>

static uint8_t s_probe_addrs[MOCK_I2C_MAX_DEVICES];
static size_t s_probe_count = 0;

static uint8_t s_rx_data[256];
static size_t s_rx_len = 0;

static uint8_t s_tx_data[MOCK_I2C_MAX_TX_BYTES];
static size_t s_tx_len = 0;

static uint16_t s_last_dev_addr = 0;
static int s_next_error = 0;
static int s_dummy_bus = 1;
static int s_dummy_dev = 2;

void mock_i2c_reset(void)
{
    s_probe_count = 0;
    memset(s_probe_addrs, 0, sizeof(s_probe_addrs));
    s_rx_len = 0;
    s_tx_len = 0;
    s_last_dev_addr = 0;
    s_next_error = 0;
}

void mock_i2c_set_probe_devices(const uint8_t *addrs, size_t count)
{
    s_probe_count = (count > MOCK_I2C_MAX_DEVICES) ? MOCK_I2C_MAX_DEVICES : count;
    memcpy(s_probe_addrs, addrs, s_probe_count);
}

void mock_i2c_set_rx_data(const uint8_t *data, size_t len)
{
    s_rx_len = (len > sizeof(s_rx_data)) ? sizeof(s_rx_data) : len;
    memcpy(s_rx_data, data, s_rx_len);
}

const uint8_t *mock_i2c_get_tx_data(size_t *len)
{
    if (len)
    {
        *len = s_tx_len;
    }
    return s_tx_data;
}

uint16_t mock_i2c_get_last_device_addr(void)
{
    return s_last_dev_addr;
}

void mock_i2c_set_next_error(int err)
{
    s_next_error = err;
}

esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *bus_config, i2c_master_bus_handle_t *ret_bus_handle)
{
    (void)bus_config;
    if (s_next_error != 0)
    {
        int err = s_next_error;
        s_next_error = 0;
        return err;
    }
    *ret_bus_handle = (i2c_master_bus_handle_t)&s_dummy_bus;
    return ESP_OK;
}

esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus_handle)
{
    (void)bus_handle;
    return ESP_OK;
}

esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus_handle, const i2c_device_config_t *dev_config,
                                    i2c_master_dev_handle_t *ret_handle)
{
    (void)bus_handle;
    if (s_next_error != 0)
    {
        int err = s_next_error;
        s_next_error = 0;
        return err;
    }
    s_last_dev_addr = dev_config->device_address;
    *ret_handle = (i2c_master_dev_handle_t)&s_dummy_dev;
    return ESP_OK;
}

esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}

esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus_handle, uint16_t address, int xfer_timeout_ms)
{
    (void)bus_handle;
    (void)xfer_timeout_ms;
    for (size_t i = 0; i < s_probe_count; i++)
    {
        if (s_probe_addrs[i] == address)
        {
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

esp_err_t i2c_master_receive(i2c_master_dev_handle_t dev_handle, uint8_t *read_buffer, size_t read_size,
                             int xfer_timeout_ms)
{
    (void)dev_handle;
    (void)xfer_timeout_ms;
    if (s_next_error != 0)
    {
        int err = s_next_error;
        s_next_error = 0;
        return err;
    }
    size_t copy = (read_size < s_rx_len) ? read_size : s_rx_len;
    memcpy(read_buffer, s_rx_data, copy);
    if (read_size > copy)
    {
        memset(read_buffer + copy, 0, read_size - copy);
    }
    return ESP_OK;
}

esp_err_t i2c_master_transmit(i2c_master_dev_handle_t dev_handle, const uint8_t *write_buffer, size_t write_size,
                              int xfer_timeout_ms)
{
    (void)dev_handle;
    (void)xfer_timeout_ms;
    if (s_next_error != 0)
    {
        int err = s_next_error;
        s_next_error = 0;
        return err;
    }
    s_tx_len = (write_size > MOCK_I2C_MAX_TX_BYTES) ? MOCK_I2C_MAX_TX_BYTES : write_size;
    memcpy(s_tx_data, write_buffer, s_tx_len);
    return ESP_OK;
}
