#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>

typedef void (*shell_line_input_callback_t)(esp_err_t result, const char *line, void *context);

/**
 * Initialize the shared input subsystem: UART RX task, input stream,
 * and the line-editor / command-dispatch task.
 * Must be called after esp_console_init() and command registration.
 */
esp_err_t shell_input_init(void);

/**
 * Request one non-blocking line from the shell input task.
 * Calls from other tasks return ESP_ERR_INVALID_STATE.
 */
esp_err_t shell_input_read_line(const char *prompt, shell_line_input_callback_t callback, void *context);

/** Cancel the pending line request by queuing a cancellation event to the input task. */
esp_err_t shell_input_cancel_line(void *context);

/** True while a non-blocking line request owns console input. */
bool shell_input_is_reading_line(void);
