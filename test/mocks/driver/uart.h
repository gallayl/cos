#pragma once

#include <stddef.h>

typedef int uart_port_t;

#define UART_NUM_0 0
#define UART_NUM_1 1
#define UART_NUM_2 2

int uart_write_bytes(uart_port_t uart_num, const void *src, size_t size);
