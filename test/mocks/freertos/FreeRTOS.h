#pragma once

#include <stdint.h>

typedef uint32_t EventBits_t;
typedef uint32_t TickType_t;
typedef int BaseType_t;

#define pdFALSE 0
#define pdTRUE  1

#define BIT0 (1 << 0)
#define BIT1 (1 << 1)

#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
