#include "mock_ledc.h"
#include "driver/ledc.h"

#include <string.h>

#define MOCK_LEDC_MAX_CHANNELS 8

static uint32_t s_duty[MOCK_LEDC_MAX_CHANNELS];

void mock_ledc_reset(void)
{
    memset(s_duty, 0, sizeof(s_duty));
}

uint32_t mock_ledc_get_duty(int channel)
{
    if (channel < 0 || channel >= MOCK_LEDC_MAX_CHANNELS)
    {
        return 0;
    }
    return s_duty[channel];
}

esp_err_t ledc_timer_config(const ledc_timer_config_t *timer_conf)
{
    (void)timer_conf;
    return ESP_OK;
}

esp_err_t ledc_channel_config(const ledc_channel_config_t *ledc_conf)
{
    if (ledc_conf == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (ledc_conf->channel >= 0 && ledc_conf->channel < MOCK_LEDC_MAX_CHANNELS)
    {
        s_duty[ledc_conf->channel] = ledc_conf->duty;
    }
    return ESP_OK;
}

esp_err_t ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty)
{
    (void)speed_mode;
    if (channel >= 0 && channel < MOCK_LEDC_MAX_CHANNELS)
    {
        s_duty[channel] = duty;
    }
    return ESP_OK;
}

esp_err_t ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t channel)
{
    (void)speed_mode;
    (void)channel;
    return ESP_OK;
}
