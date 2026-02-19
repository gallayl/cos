#pragma once

#include <stddef.h>
#include <stdint.h>

/** Reset mock UART state (clear captured data). */
void mock_uart_reset(void);

/** Return pointer to the bytes written so far and their count. */
const uint8_t *mock_uart_get_written_data(size_t *out_len);
