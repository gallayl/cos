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
