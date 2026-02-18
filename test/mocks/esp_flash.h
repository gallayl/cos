#pragma once

#include "esp_err.h"

#include <stdint.h>

typedef void *esp_flash_t;

esp_err_t esp_flash_get_size(esp_flash_t *chip, uint32_t *out_size);
