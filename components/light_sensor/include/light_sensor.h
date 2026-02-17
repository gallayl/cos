#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize the light sensor ADC and register shell commands.
     */
    esp_err_t light_sensor_init(void);

    /**
     * Read the current ambient light level.
     * @return Raw ADC value (0-4095), or -1 on error.
     */
    int light_sensor_read(void);

#ifdef __cplusplus
}
#endif
