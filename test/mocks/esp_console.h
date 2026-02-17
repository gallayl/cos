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

static inline esp_err_t esp_console_cmd_register(const esp_console_cmd_t *cmd)
{
    (void)cmd;
    return ESP_OK;
}

/* REPL stubs for host tests */

typedef struct
{
    int dummy;
} esp_console_repl_t;

typedef struct
{
    const char *prompt;
    size_t max_cmdline_length;
} esp_console_repl_config_t;

typedef struct
{
    int channel;
} esp_console_dev_uart_config_t;

#define ESP_CONSOLE_REPL_CONFIG_DEFAULT() \
    {                                     \
        .prompt = "cos> ",                \
        .max_cmdline_length = 256,        \
    }

#define ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT() \
    {                                         \
        .channel = 0,                         \
    }

static inline esp_err_t esp_console_new_repl_uart(const esp_console_dev_uart_config_t *dev_config,
                                                   const esp_console_repl_config_t *repl_config,
                                                   esp_console_repl_t **ret_repl)
{
    (void)dev_config;
    (void)repl_config;
    static esp_console_repl_t s_repl;
    *ret_repl = &s_repl;
    return ESP_OK;
}

static inline esp_err_t esp_console_start_repl(esp_console_repl_t *repl)
{
    (void)repl;
    return ESP_OK;
}
