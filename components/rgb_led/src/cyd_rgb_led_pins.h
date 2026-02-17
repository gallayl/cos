#pragma once

// CYD Board: ESP32-2432S028R -- RGB LED (active-low, accent LED on PCB)

#define CYD_RGB_LED_PIN_R 4
#define CYD_RGB_LED_PIN_G 16
#define CYD_RGB_LED_PIN_B 17

// LEDC PWM configuration (does not conflict with display backlight on channel 7)
#define CYD_RGB_LED_LEDC_TIMER LEDC_TIMER_0
#define CYD_RGB_LED_LEDC_SPEED LEDC_LOW_SPEED_MODE
#define CYD_RGB_LED_LEDC_DUTY_RES LEDC_TIMER_8_BIT
#define CYD_RGB_LED_LEDC_FREQ 5000
#define CYD_RGB_LED_LEDC_CH_R LEDC_CHANNEL_0
#define CYD_RGB_LED_LEDC_CH_G LEDC_CHANNEL_1
#define CYD_RGB_LED_LEDC_CH_B LEDC_CHANNEL_2
