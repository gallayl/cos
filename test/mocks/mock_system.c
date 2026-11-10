#include "mock_system.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_system.h"

static uint32_t s_free_heap = 200000;
static uint32_t s_min_free_heap = 150000;
static size_t s_total_heap = 327680;
static size_t s_max_alloc = 113792;
static uint32_t s_internal_free_heap = 200000;
static uint32_t s_internal_min_free_heap = 150000;
static size_t s_internal_total_heap = 327680;
static size_t s_internal_max_alloc = 113792;
static uint32_t s_flash_size = 4194304;
static int64_t s_uptime_us = 0;
static bool s_restart_called = false;

void mock_system_reset(void)
{
    s_free_heap = 200000;
    s_min_free_heap = 150000;
    s_total_heap = 327680;
    s_max_alloc = 113792;
    s_internal_free_heap = 200000;
    s_internal_min_free_heap = 150000;
    s_internal_total_heap = 327680;
    s_internal_max_alloc = 113792;
    s_flash_size = 4194304;
    s_uptime_us = 0;
    s_restart_called = false;
}

void mock_system_set_free_heap(uint32_t bytes)
{
    s_free_heap = bytes;
    s_internal_free_heap = bytes;
}

void mock_system_set_min_free_heap(uint32_t bytes)
{
    s_min_free_heap = bytes;
    s_internal_min_free_heap = bytes;
}

void mock_system_set_total_heap(size_t bytes)
{
    s_total_heap = bytes;
    s_internal_total_heap = bytes;
}

void mock_system_set_max_alloc(size_t bytes)
{
    s_max_alloc = bytes;
    s_internal_max_alloc = bytes;
}

void mock_system_set_flash_size(uint32_t bytes)
{
    s_flash_size = bytes;
}

void mock_system_set_internal_free_heap(uint32_t bytes)
{
    s_internal_free_heap = bytes;
}

void mock_system_set_internal_min_free_heap(uint32_t bytes)
{
    s_internal_min_free_heap = bytes;
}

void mock_system_set_internal_total_heap(size_t bytes)
{
    s_internal_total_heap = bytes;
}

void mock_system_set_internal_max_alloc(size_t bytes)
{
    s_internal_max_alloc = bytes;
}

void mock_system_set_uptime_us(int64_t us)
{
    s_uptime_us = us;
}

bool mock_system_restart_called(void)
{
    return s_restart_called;
}

/* --- esp_system.h stubs --- */

const char *esp_get_idf_version(void)
{
    return "v6.1-mock";
}

uint32_t esp_get_free_heap_size(void)
{
    return s_free_heap;
}

uint32_t esp_get_minimum_free_heap_size(void)
{
    return s_min_free_heap;
}

void esp_restart(void)
{
    s_restart_called = true;
}

/* --- esp_flash.h stubs --- */

esp_err_t esp_flash_get_size(esp_flash_t *chip, uint32_t *out_size)
{
    (void)chip;
    if (out_size)
    {
        *out_size = s_flash_size;
    }
    return ESP_OK;
}

/* --- esp_heap_caps.h stubs --- */

static bool mock_heap_caps_internal(uint32_t caps)
{
    return (caps & MALLOC_CAP_INTERNAL) != 0;
}

size_t heap_caps_get_total_size(uint32_t caps)
{
    return mock_heap_caps_internal(caps) ? s_internal_total_heap : s_total_heap;
}

size_t heap_caps_get_free_size(uint32_t caps)
{
    return mock_heap_caps_internal(caps) ? (size_t)s_internal_free_heap : (size_t)s_free_heap;
}

size_t heap_caps_get_minimum_free_size(uint32_t caps)
{
    return mock_heap_caps_internal(caps) ? (size_t)s_internal_min_free_heap : (size_t)s_min_free_heap;
}

size_t heap_caps_get_largest_free_block(uint32_t caps)
{
    return mock_heap_caps_internal(caps) ? s_internal_max_alloc : s_max_alloc;
}

/* --- esp_timer_get_time stub --- */

int64_t esp_timer_get_time(void)
{
    return s_uptime_us;
}
