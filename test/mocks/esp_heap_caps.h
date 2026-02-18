#pragma once

#include <stddef.h>
#include <stdint.h>

#define MALLOC_CAP_DEFAULT (1 << 0)

size_t heap_caps_get_total_size(uint32_t caps);
size_t heap_caps_get_largest_free_block(uint32_t caps);
