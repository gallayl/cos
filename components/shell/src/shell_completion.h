#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>

esp_err_t shell_complete_line(char *line, size_t capacity, bool *changed);
