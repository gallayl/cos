#include "mock_esp_timer.h"
#include "esp_timer.h"

#include <stddef.h>

static esp_timer_cb_t s_callback = NULL;
static void *s_callback_arg = NULL;
static bool s_running = false;
static uint64_t s_period_us = 0;
static int s_dummy_handle = 42;

void mock_esp_timer_reset(void)
{
    s_callback = NULL;
    s_callback_arg = NULL;
    s_running = false;
    s_period_us = 0;
}

bool mock_esp_timer_is_running(void)
{
    return s_running;
}

uint64_t mock_esp_timer_get_period_us(void)
{
    return s_period_us;
}

void mock_esp_timer_fire(void)
{
    if (s_callback != NULL)
    {
        s_callback(s_callback_arg);
    }
}

esp_err_t esp_timer_create(const esp_timer_create_args_t *create_args, esp_timer_handle_t *out_handle)
{
    if (create_args == NULL || out_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    s_callback = create_args->callback;
    s_callback_arg = create_args->arg;
    *out_handle = (esp_timer_handle_t)&s_dummy_handle;
    return ESP_OK;
}

esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period)
{
    (void)timer;
    if (s_running)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_period_us = period;
    s_running = true;
    return ESP_OK;
}

esp_err_t esp_timer_stop(esp_timer_handle_t timer)
{
    (void)timer;
    s_running = false;
    return ESP_OK;
}

esp_err_t esp_timer_delete(esp_timer_handle_t timer)
{
    (void)timer;
    s_running = false;
    s_callback = NULL;
    return ESP_OK;
}
