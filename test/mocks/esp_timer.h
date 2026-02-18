#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef void *esp_timer_handle_t;

typedef enum
{
    ESP_TIMER_TASK = 0,
} esp_timer_dispatch_t;

typedef void (*esp_timer_cb_t)(void *arg);

typedef struct
{
    esp_timer_cb_t callback;
    void *arg;
    esp_timer_dispatch_t dispatch_method;
    const char *name;
} esp_timer_create_args_t;

esp_err_t esp_timer_create(const esp_timer_create_args_t *create_args, esp_timer_handle_t *out_handle);
esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period);
esp_err_t esp_timer_stop(esp_timer_handle_t timer);
esp_err_t esp_timer_delete(esp_timer_handle_t timer);

int64_t esp_timer_get_time(void);
