#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize the calibration subsystem.
     * Loads any stored calibration for the current rotation and
     * registers the "calibrate" console command.
     */
    esp_err_t calibration_init(void);

    /**
     * Load stored calibration data for the current display rotation.
     * Returns ESP_ERR_NOT_FOUND if no calibration exists for this rotation.
     */
    esp_err_t calibration_load(void);

    /**
     * Run the interactive touch calibration routine.
     * Blocks until the user completes calibration, then saves the result
     * to NVS for the current rotation.
     */
    esp_err_t calibration_run(void);

#ifdef __cplusplus
}
#endif
