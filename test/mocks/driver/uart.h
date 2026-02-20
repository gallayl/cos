#pragma once

#include "esp_err.h"

#include <stddef.h>

typedef int uart_port_t;
typedef void *QueueHandle_t;

#define UART_NUM_0 0
#define UART_NUM_1 1
#define UART_NUM_2 2

#ifndef CONFIG_ESP_CONSOLE_UART_NUM
#define CONFIG_ESP_CONSOLE_UART_NUM 0
#endif

int uart_write_bytes(uart_port_t uart_num, const void *src, size_t size);
int uart_read_bytes(uart_port_t uart_num, void *buf, uint32_t length, uint32_t ticks_to_wait);
esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size,
                              QueueHandle_t *uart_queue, int intr_alloc_flags);
