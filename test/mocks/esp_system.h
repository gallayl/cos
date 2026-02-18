#pragma once

#include <stdint.h>

const char *esp_get_idf_version(void);
uint32_t esp_get_free_heap_size(void);
uint32_t esp_get_minimum_free_heap_size(void);
void esp_restart(void);
