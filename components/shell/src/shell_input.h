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

/** Cancel the pending line, key, or character request by queuing a cancellation event. */
esp_err_t shell_input_cancel_line(void *context);

/** True while a line, key, or character request owns console input. */
bool shell_input_is_reading_line(void);

/**
 * Request the next key, including arrows and editing keys.
 * The callback receives a name such as "up" or a one-character string.
 * Ctrl+C completes with ESP_ERR_INVALID_STATE and a NULL key. Nothing is echoed.
 * Calls from other tasks return ESP_ERR_INVALID_STATE.
 */
esp_err_t shell_input_read_key(shell_line_input_callback_t callback, void *context);

/**
 * Request the next printable ASCII character.
 * Arrows, enter, and other named keys are discarded until a character arrives.
 * Ctrl+C completes with ESP_ERR_INVALID_STATE and a NULL character.
 */
esp_err_t shell_input_read_char(shell_line_input_callback_t callback, void *context);
