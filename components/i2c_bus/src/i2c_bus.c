#include "i2c_bus.h"
#include "cyd_i2c_pins.h"

#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *const TAG = "i2c_bus";

static i2c_master_bus_handle_t s_bus_handle;

void i2c_bus_register_commands(void);

esp_err_t i2c_bus_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = CYD_I2C_PORT_NUM,
        .sda_io_num = CYD_I2C_SDA_PIN,
        .scl_io_num = CYD_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = CYD_I2C_GLITCH_IGNORE,
        .flags = {.enable_internal_pullup = true},
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to init I2C bus: %s", esp_err_to_name(err));
        return err;
    }

    i2c_bus_register_commands();

    ESP_LOGI(TAG, "I2C bus initialized (SDA=%d, SCL=%d, %d Hz)", CYD_I2C_SDA_PIN, CYD_I2C_SCL_PIN, CYD_I2C_FREQ_HZ);
    return ESP_OK;
}

esp_err_t i2c_bus_scan(uint8_t *addrs, size_t max_addrs, size_t *found)
{
    if (addrs == NULL || found == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *found = 0;
    for (uint16_t addr = 1; addr < 127; addr++)
    {
        esp_err_t err = i2c_master_probe(s_bus_handle, addr, 50);
        if (err == ESP_OK)
        {
            if (*found < max_addrs)
            {
                addrs[*found] = (uint8_t)addr;
            }
            (*found)++;
        }
    }
    return ESP_OK;
}

esp_err_t i2c_bus_read(uint8_t addr, uint8_t *buf, size_t len)
{
    if (buf == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = CYD_I2C_FREQ_HZ,
    };

    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &dev);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Add device 0x%02X failed: %s", addr, esp_err_to_name(err));
        return err;
    }

    err = i2c_master_receive(dev, buf, len, 100);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Read from 0x%02X failed: %s", addr, esp_err_to_name(err));
    }

    i2c_master_bus_rm_device(dev);
    return err;
}

esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = CYD_I2C_FREQ_HZ,
    };

    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &dev);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Add device 0x%02X failed: %s", addr, esp_err_to_name(err));
        return err;
    }

    err = i2c_master_transmit(dev, data, len, 100);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Write to 0x%02X failed: %s", addr, esp_err_to_name(err));
    }

    i2c_master_bus_rm_device(dev);
    return err;
}
