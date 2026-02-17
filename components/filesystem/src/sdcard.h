#pragma once

#include "esp_err.h"

#define SDCARD_MOUNT_POINT "/sdcard"

esp_err_t sdcard_init(void);
