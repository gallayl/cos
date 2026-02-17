#pragma once

#include <stddef.h>

#include "esp_console.h"
#include "esp_err.h"

#ifndef __containerof
#define __containerof(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#define CONSOLE_PROMPT_MAX_LEN (32)

typedef enum { REPL_STATE_INIT, REPL_STATE_STARTED } repl_state_t;

typedef struct
{
    esp_console_repl_t repl_core;
    char prompt[CONSOLE_PROMPT_MAX_LEN];
    repl_state_t state;
    void *state_mux;
    const char *history_save_path;
    void *task_hdl;
    size_t max_cmdline_length;
} esp_console_repl_com_t;
