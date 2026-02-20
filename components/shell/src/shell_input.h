#pragma once

#include "esp_err.h"

/**
 * Initialize the shared input subsystem: UART RX task, input stream,
 * and the line-editor / command-dispatch task.
 * Must be called after esp_console_init() and command registration.
 */
esp_err_t shell_input_init(void);
