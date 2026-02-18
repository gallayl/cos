#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MOCK_I2C_MAX_DEVICES 16
#define MOCK_I2C_MAX_TX_BYTES 256

void mock_i2c_reset(void);

/** Configure which addresses will ACK during a scan. */
void mock_i2c_set_probe_devices(const uint8_t *addrs, size_t count);

/** Set data that the next i2c_master_receive() call will return. */
void mock_i2c_set_rx_data(const uint8_t *data, size_t len);

/** Get the data from the last i2c_master_transmit() call. */
const uint8_t *mock_i2c_get_tx_data(size_t *len);

/** Get the device address used in the last add_device call. */
uint16_t mock_i2c_get_last_device_addr(void);

/** Make the next bus operation fail with the given error. */
void mock_i2c_set_next_error(int err);
