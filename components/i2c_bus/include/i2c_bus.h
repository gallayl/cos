#pragma once

#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Maximum number of devices on a 7-bit I2C bus (addresses 1..126). */
#define I2C_BUS_MAX_DEVICES 126

    /**
     * @brief Initialize the I2C master bus and register shell commands.
     *
     * Configures the bus with CYD board pin definitions and enables
     * internal pull-ups.
     */
    esp_err_t i2c_bus_init(void);

    /**
     * @brief Scan the I2C bus for responding devices.
     *
     * Probes addresses 1 through 126 and stores each address that
     * acknowledges into the caller-provided buffer.
     *
     * @param addrs      Output buffer for discovered device addresses.
     * @param max_addrs  Capacity of the output buffer.
     * @param found      On return, number of devices found.
     * @return ESP_OK on success, or an error code.
     */
    esp_err_t i2c_bus_scan(uint8_t *addrs, size_t max_addrs, size_t *found);

    /**
     * @brief Read bytes from an I2C device.
     *
     * @param addr 7-bit device address.
     * @param buf  Buffer to receive data.
     * @param len  Number of bytes to read.
     * @return ESP_OK on success, or an error code.
     */
    esp_err_t i2c_bus_read(uint8_t addr, uint8_t *buf, size_t len);

    /**
     * @brief Write bytes to an I2C device.
     *
     * @param addr 7-bit device address.
     * @param data Bytes to transmit.
     * @param len  Number of bytes to write.
     * @return ESP_OK on success, or an error code.
     */
    esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
