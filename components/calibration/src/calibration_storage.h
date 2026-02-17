#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#include "display.h"

#define CALIBRATION_NVS_NAMESPACE "calibration"
#define CALIBRATION_KEY_PREFIX "cal_rot_"
#define CALIBRATION_MAX_ROTATION 7

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Save calibration data for a given rotation to NVS.
     * @param rotation Display rotation (0-7)
     * @param cal_data Array of 8 uint16_t calibration values
     */
    esp_err_t calibration_storage_save(uint8_t rotation, const uint16_t cal_data[DISPLAY_CAL_DATA_LEN]);

    /**
     * Load calibration data for a given rotation from NVS.
     * @param rotation Display rotation (0-7)
     * @param cal_data Output array of 8 uint16_t calibration values
     * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not stored
     */
    esp_err_t calibration_storage_load(uint8_t rotation, uint16_t cal_data[DISPLAY_CAL_DATA_LEN]);

    /**
     * Check whether calibration data exists in NVS for a given rotation.
     */
    bool calibration_storage_exists(uint8_t rotation);

#ifdef __cplusplus
}
#endif
