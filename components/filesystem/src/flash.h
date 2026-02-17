#pragma once

#include "esp_err.h"

#include <stddef.h>

#define FLASH_MOUNT_POINT     "/littlefs"
#define FLASH_PARTITION_LABEL "littlefs"

esp_err_t flash_init(void);
void flash_deinit(void);
