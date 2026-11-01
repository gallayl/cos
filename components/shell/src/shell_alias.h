#pragma once

#include "esp_err.h"

#include <stddef.h>

esp_err_t shell_alias_init(void);
esp_err_t shell_alias_expand(const char *line, char *expanded, size_t expanded_size);
