#include "mock_light_sensor.h"
#include "light_sensor.h"

static int s_value = 0;

void mock_light_sensor_reset(void)
{
    s_value = 0;
}

void mock_light_sensor_set_value(int value)
{
    s_value = value;
}

esp_err_t light_sensor_init(void)
{
    return ESP_OK;
}

int light_sensor_read(void)
{
    return s_value;
}
