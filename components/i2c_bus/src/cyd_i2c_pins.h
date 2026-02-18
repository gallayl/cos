#pragma once

// CYD Board: ESP32-2432S028R -- I2C bus
// SDA = GPIO 27, SCL = GPIO 22
// (GPIO 21 is display backlight -- do not use for I2C)

#define CYD_I2C_SDA_PIN 27
#define CYD_I2C_SCL_PIN 22
#define CYD_I2C_PORT_NUM 0
#define CYD_I2C_FREQ_HZ 100000
#define CYD_I2C_GLITCH_IGNORE 7
