#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize OTA: register /update endpoints on the HTTP server
     *        and register the ota shell command.
     */
    esp_err_t ota_init(void);

#ifdef __cplusplus
}
#endif
