#pragma once

#include <stddef.h>

#include "esp_err.h"

typedef int (*esp_console_cmd_func_t)(int argc, char **argv);

typedef struct
{
    const char *command;
    const char *help;
    const char *hint;
    esp_console_cmd_func_t func;
} esp_console_cmd_t;

esp_err_t esp_console_cmd_register(const esp_console_cmd_t *cmd);

/* Console init stubs */

typedef struct
{
    size_t max_cmdline_length;
    size_t max_cmdline_args;
} esp_console_config_t;

#define ESP_CONSOLE_CONFIG_DEFAULT() \
    {                                \
        .max_cmdline_length = 256,   \
        .max_cmdline_args = 8,       \
    }

static inline esp_err_t esp_console_init(const esp_console_config_t *config)
{
    (void)config;
    return ESP_OK;
}

static inline esp_err_t esp_console_run(const char *cmdline, int *cmd_ret)
{
    (void)cmdline;
    (void)cmd_ret;
    return ESP_OK;
}
