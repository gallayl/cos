#include "calibration_storage.h"

#include "esp_log.h"
#include "nvs.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "cal_storage";

static esp_err_t make_key(uint8_t rotation, char *buf, size_t buf_len)
{
    if (rotation > CALIBRATION_MAX_ROTATION)
    {
        return ESP_ERR_INVALID_ARG;
    }
    int written = snprintf(buf, buf_len, "%s%u", CALIBRATION_KEY_PREFIX, rotation);
    if (written < 0 || (size_t)written >= buf_len)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t calibration_storage_save(uint8_t rotation, const uint16_t cal_data[DISPLAY_CAL_DATA_LEN])
{
    if (cal_data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char key[16];
    esp_err_t err = make_key(rotation, key, sizeof(key));
    if (err != ESP_OK)
    {
        return err;
    }

    nvs_handle_t handle;
    err = nvs_open(CALIBRATION_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(handle, key, cal_data, DISPLAY_CAL_DATA_LEN * sizeof(uint16_t));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write calibration: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Saved calibration for rotation %u", rotation);
    }
    return err;
}

esp_err_t calibration_storage_load(uint8_t rotation, uint16_t cal_data[DISPLAY_CAL_DATA_LEN])
{
    if (cal_data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char key[16];
    esp_err_t err = make_key(rotation, key, sizeof(key));
    if (err != ESP_OK)
    {
        return err;
    }

    nvs_handle_t handle;
    err = nvs_open(CALIBRATION_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        return err;
    }

    size_t required_size = DISPLAY_CAL_DATA_LEN * sizeof(uint16_t);
    err = nvs_get_blob(handle, key, cal_data, &required_size);
    nvs_close(handle);

    if (err == ESP_OK && required_size != DISPLAY_CAL_DATA_LEN * sizeof(uint16_t))
    {
        ESP_LOGE(TAG, "Calibration data size mismatch for rotation %u", rotation);
        return ESP_ERR_INVALID_SIZE;
    }

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Loaded calibration for rotation %u", rotation);
    }
    return err;
}

bool calibration_storage_exists(uint8_t rotation)
{
    char key[16];
    if (make_key(rotation, key, sizeof(key)) != ESP_OK)
    {
        return false;
    }

    nvs_handle_t handle;
    if (nvs_open(CALIBRATION_NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
    {
        return false;
    }

    size_t required_size = 0;
    esp_err_t err = nvs_get_blob(handle, key, NULL, &required_size);
    nvs_close(handle);

    return err == ESP_OK && required_size == DISPLAY_CAL_DATA_LEN * sizeof(uint16_t);
}
