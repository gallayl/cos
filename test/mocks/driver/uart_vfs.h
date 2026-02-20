#pragma once

#include "driver/uart.h"

static inline void uart_vfs_dev_use_driver(uart_port_t uart_num)
{
    (void)uart_num;
}
