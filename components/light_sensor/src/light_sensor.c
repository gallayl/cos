#include "light_sensor.h"
#include "cyd_light_sensor_pins.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *const TAG = "light_sensor";

static adc_oneshot_unit_handle_t s_adc_handle = NULL;

void light_sensor_register_commands(void);

esp_err_t light_sensor_init(void)
{
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = CYD_LIGHT_SENSOR_ADC_UNIT,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_cfg, &s_adc_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to init ADC unit: %s", esp_err_to_name(err));
        return err;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = CYD_LIGHT_SENSOR_ADC_ATTEN,
        .bitwidth = CYD_LIGHT_SENSOR_ADC_BITWIDTH,
    };
    err = adc_oneshot_config_channel(s_adc_handle, CYD_LIGHT_SENSOR_ADC_CHANNEL, &chan_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(err));
        return err;
    }

    light_sensor_register_commands();

    ESP_LOGI(TAG, "Light sensor initialized");
    return ESP_OK;
}

int light_sensor_read(void)
{
    if (s_adc_handle == NULL)
    {
        return -1;
    }

    int raw = 0;
    esp_err_t err = adc_oneshot_read(s_adc_handle, CYD_LIGHT_SENSOR_ADC_CHANNEL, &raw);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(err));
        return -1;
    }
    return raw;
}
