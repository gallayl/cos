#pragma once

#include <stdint.h>

typedef enum
{
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA,
    WIFI_MODE_MAX
} wifi_mode_t;

typedef enum
{
    WIFI_IF_STA = 0,
    WIFI_IF_AP,
    WIFI_IF_MAX
} wifi_interface_t;

typedef enum
{
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_WEP,
    WIFI_AUTH_WPA_PSK,
    WIFI_AUTH_WPA2_PSK,
    WIFI_AUTH_WPA_WPA2_PSK,
    WIFI_AUTH_WPA2_ENTERPRISE,
    WIFI_AUTH_WPA3_PSK,
    WIFI_AUTH_WPA2_WPA3_PSK,
    WIFI_AUTH_MAX
} wifi_auth_mode_t;

typedef struct
{
    uint8_t ssid[33];
    uint8_t password[64];
} wifi_sta_config_t;

typedef struct
{
    uint8_t ssid[33];
    uint8_t password[64];
    uint8_t ssid_len;
    uint8_t max_connection;
    wifi_auth_mode_t authmode;
} wifi_ap_config_t;

typedef union
{
    wifi_sta_config_t sta;
    wifi_ap_config_t ap;
} wifi_config_t;

typedef struct
{
    uint8_t ssid[33];
    int8_t rssi;
    wifi_auth_mode_t authmode;
} wifi_ap_record_t;

typedef struct
{
    int show_hidden;
} wifi_scan_config_t;

typedef struct
{
    int magic;
} wifi_init_config_t;

#define WIFI_INIT_CONFIG_DEFAULT() \
    {                              \
        .magic = 0x1F2F3F,        \
    }
