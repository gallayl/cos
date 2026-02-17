#include "calibration.h"
#include "calibration_storage.h"

#include "display.h"
#include "esp_log.h"
#include "nvs.h"

static const char *const TAG = "calibration";

void calibration_register_commands(void);

esp_err_t calibration_load(void)
{
    uint8_t rotation = display_get_rotation();
    uint16_t cal_data[DISPLAY_CAL_DATA_LEN];

    esp_err_t err = calibration_storage_load(rotation, cal_data);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "No calibration data for rotation %u", rotation);
        return ESP_ERR_NOT_FOUND;
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to load calibration: %s", esp_err_to_name(err));
        return err;
    }

    err = display_set_touch_calibration(cal_data);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to apply calibration: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Calibration applied for rotation %u", rotation);
    return ESP_OK;
}

esp_err_t calibration_run(void)
{
    uint16_t cal_data[DISPLAY_CAL_DATA_LEN];

    esp_err_t err = display_calibrate_touch(cal_data);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Interactive calibration failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t rotation = display_get_rotation();
    err = calibration_storage_save(rotation, cal_data);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save calibration: %s", esp_err_to_name(err));
        return err;
    }

    err = display_set_touch_calibration(cal_data);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to apply new calibration: %s", esp_err_to_name(err));
        return err;
    }

    display_fill_screen(0x0000);
    display_wait();

    ESP_LOGI(TAG, "Calibration complete and saved for rotation %u", rotation);
    return ESP_OK;
}

esp_err_t calibration_init(void)
{
    calibration_register_commands();

    esp_err_t err = calibration_load();
    if (err == ESP_ERR_NOT_FOUND)
    {
        ESP_LOGI(TAG, "No stored calibration found, run 'calibrate' command");
        return ESP_OK;
    }

    return err;
}
