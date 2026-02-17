#include "mock_display.h"

#include <string.h>

static uint8_t mock_rotation = 0;
static uint8_t mock_brightness = 128;
static uint16_t mock_cal_result[DISPLAY_CAL_DATA_LEN];
static uint16_t mock_applied_cal[DISPLAY_CAL_DATA_LEN];
static int cal_count = 0;
static int set_cal_count = 0;
static int fill_count = 0;

void mock_display_reset(void)
{
    mock_rotation = 0;
    mock_brightness = 128;
    memset(mock_cal_result, 0, sizeof(mock_cal_result));
    memset(mock_applied_cal, 0, sizeof(mock_applied_cal));
    cal_count = 0;
    set_cal_count = 0;
    fill_count = 0;
}

void mock_display_set_rotation(uint8_t rotation)
{
    mock_rotation = rotation;
}

void mock_display_set_cal_result(const uint16_t cal_data[DISPLAY_CAL_DATA_LEN])
{
    memcpy(mock_cal_result, cal_data, sizeof(mock_cal_result));
}

const uint16_t *mock_display_get_applied_cal(void)
{
    return mock_applied_cal;
}

int mock_display_calibrate_call_count(void)
{
    return cal_count;
}

int mock_display_set_cal_call_count(void)
{
    return set_cal_count;
}

int mock_display_fill_screen_call_count(void)
{
    return fill_count;
}

uint8_t mock_display_get_brightness(void)
{
    return mock_brightness;
}

/* --- Display API mock implementations --- */

esp_err_t display_init(void)
{
    return ESP_OK;
}

uint8_t display_get_rotation(void)
{
    return mock_rotation;
}

esp_err_t display_set_rotation(uint8_t rotation)
{
    if (rotation > 7)
    {
        return ESP_ERR_INVALID_ARG;
    }
    mock_rotation = rotation;
    return ESP_OK;
}

esp_err_t display_calibrate_touch(uint16_t cal_data[DISPLAY_CAL_DATA_LEN])
{
    if (cal_data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    cal_count++;
    memcpy(cal_data, mock_cal_result, sizeof(mock_cal_result));
    return ESP_OK;
}

esp_err_t display_set_touch_calibration(const uint16_t cal_data[DISPLAY_CAL_DATA_LEN])
{
    if (cal_data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    set_cal_count++;
    memcpy(mock_applied_cal, cal_data, sizeof(mock_applied_cal));
    return ESP_OK;
}

esp_err_t display_set_brightness(uint8_t brightness)
{
    mock_brightness = brightness;
    return ESP_OK;
}

uint8_t display_get_brightness(void)
{
    return mock_brightness;
}

void display_fill_screen(uint16_t color)
{
    (void)color;
    fill_count++;
}

void display_draw_text(int x, int y, const char *text, uint16_t fg, uint16_t bg)
{
    (void)x;
    (void)y;
    (void)text;
    (void)fg;
    (void)bg;
}

void display_wait(void)
{
}
