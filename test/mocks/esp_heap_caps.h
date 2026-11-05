#pragma once

#include <stddef.h>
#include <stdint.h>

#define MALLOC_CAP_DEFAULT (1 << 0)
#define MALLOC_CAP_INTERNAL (1 << 1)
#define MALLOC_CAP_8BIT (1 << 2)

size_t heap_caps_get_free_size(uint32_t caps);
size_t heap_caps_get_minimum_free_size(uint32_t caps);
size_t heap_caps_get_total_size(uint32_t caps);
size_t heap_caps_get_largest_free_block(uint32_t caps);
