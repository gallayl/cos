#include "mock_uart.h"
#include "driver/uart.h"

#include <string.h>

#define MOCK_UART_BUF_SIZE 1024

static uint8_t s_buf[MOCK_UART_BUF_SIZE];
static size_t s_buf_len;

void mock_uart_reset(void)
{
    s_buf_len = 0;
    memset(s_buf, 0, sizeof(s_buf));
}

const uint8_t *mock_uart_get_written_data(size_t *out_len)
{
    if (out_len != NULL)
    {
        *out_len = s_buf_len;
    }
    return s_buf;
}

int uart_write_bytes(uart_port_t uart_num, const void *src, size_t size)
{
    (void)uart_num;
    if (src == NULL || size == 0)
    {
        return 0;
    }
    size_t to_copy = size;
    if (s_buf_len + to_copy > MOCK_UART_BUF_SIZE)
    {
        to_copy = MOCK_UART_BUF_SIZE - s_buf_len;
    }
    memcpy(&s_buf[s_buf_len], src, to_copy);
    s_buf_len += to_copy;
    return (int)to_copy;
}

int uart_read_bytes(uart_port_t uart_num, void *buf, uint32_t length, uint32_t ticks_to_wait)
{
    (void)uart_num;
    (void)buf;
    (void)length;
    (void)ticks_to_wait;
    return 0;
}

esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size,
                              QueueHandle_t *uart_queue, int intr_alloc_flags)
{
    (void)uart_num;
    (void)rx_buffer_size;
    (void)tx_buffer_size;
    (void)queue_size;
    (void)uart_queue;
    (void)intr_alloc_flags;
    return ESP_OK;
}
