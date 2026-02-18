#pragma once

#include "esp_err.h"

#include <stdint.h>

typedef struct esp_netif_obj esp_netif_t;

typedef struct
{
    struct
    {
        uint32_t addr;
    } ip;
    struct
    {
        uint32_t addr;
    } netmask;
    struct
    {
        uint32_t addr;
    } gw;
} esp_netif_ip_info_t;

typedef struct
{
    esp_netif_ip_info_t ip_info;
} ip_event_got_ip_t;

#define IPSTR          "%d.%d.%d.%d"
#define IP2STR(ipaddr) ((ipaddr)->addr & 0xFF), (((ipaddr)->addr >> 8) & 0xFF), \
                       (((ipaddr)->addr >> 16) & 0xFF), (((ipaddr)->addr >> 24) & 0xFF)

esp_err_t esp_netif_init(void);
esp_netif_t *esp_netif_create_default_wifi_sta(void);
esp_netif_t *esp_netif_create_default_wifi_ap(void);
esp_err_t esp_netif_get_ip_info(esp_netif_t *esp_netif, esp_netif_ip_info_t *ip_info);
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *if_key);
