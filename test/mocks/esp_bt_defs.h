#pragma once

#include <stdint.h>

typedef uint8_t esp_bd_addr_t[6];

#define ESP_BD_ADDR_LEN 6

#define ESP_BD_ADDR_STR         "%02x:%02x:%02x:%02x:%02x:%02x"
#define ESP_BD_ADDR_HEX(addr)   addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

#define ESP_BT_GAP_MAX_BDNAME_LEN 248
