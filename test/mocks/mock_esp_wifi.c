#include "mock_esp_wifi.h"
#include "esp_wifi.h"

#include <stdbool.h>
#include <string.h>

#define MOCK_SCAN_MAX 32

static wifi_mode_t s_mode = WIFI_MODE_NULL;
static wifi_config_t s_sta_cfg = {};
static wifi_config_t s_ap_cfg = {};
static bool s_started = false;
static int s_connect_count = 0;
static int s_disconnect_count = 0;

static wifi_ap_record_t s_scan_results[MOCK_SCAN_MAX];
static uint16_t s_scan_count = 0;

static wifi_ap_record_t s_ap_info = {};
static bool s_ap_info_set = false;

void mock_wifi_reset(void)
{
    s_mode = WIFI_MODE_NULL;
    memset(&s_sta_cfg, 0, sizeof(s_sta_cfg));
    memset(&s_ap_cfg, 0, sizeof(s_ap_cfg));
    s_started = false;
    s_connect_count = 0;
    s_disconnect_count = 0;
    s_scan_count = 0;
    memset(s_scan_results, 0, sizeof(s_scan_results));
    memset(&s_ap_info, 0, sizeof(s_ap_info));
    s_ap_info_set = false;
}

void mock_wifi_set_stored_sta_config(const char *ssid, const char *password)
{
    memset(&s_sta_cfg, 0, sizeof(s_sta_cfg));
    if (ssid != NULL)
    {
        strncpy((char *)s_sta_cfg.sta.ssid, ssid, sizeof(s_sta_cfg.sta.ssid) - 1);
    }
    if (password != NULL)
    {
        strncpy((char *)s_sta_cfg.sta.password, password, sizeof(s_sta_cfg.sta.password) - 1);
    }
}

void mock_wifi_set_scan_results(const wifi_ap_record_t *records, uint16_t count)
{
    s_scan_count = (count > MOCK_SCAN_MAX) ? MOCK_SCAN_MAX : count;
    if (s_scan_count > 0 && records != NULL)
    {
        memcpy(s_scan_results, records, s_scan_count * sizeof(wifi_ap_record_t));
    }
}

void mock_wifi_set_ap_info(const wifi_ap_record_t *info)
{
    if (info != NULL)
    {
        s_ap_info = *info;
        s_ap_info_set = true;
    }
}

wifi_mode_t mock_wifi_get_mode(void)
{
    return s_mode;
}

const wifi_config_t *mock_wifi_get_sta_config(void)
{
    return &s_sta_cfg;
}

const wifi_config_t *mock_wifi_get_ap_config(void)
{
    return &s_ap_cfg;
}

int mock_wifi_get_connect_count(void)
{
    return s_connect_count;
}

int mock_wifi_get_disconnect_count(void)
{
    return s_disconnect_count;
}

/* --- esp_wifi API stubs --- */

esp_err_t esp_wifi_init(const wifi_init_config_t *config)
{
    (void)config;
    return ESP_OK;
}

esp_err_t esp_wifi_start(void)
{
    s_started = true;
    return ESP_OK;
}

esp_err_t esp_wifi_stop(void)
{
    s_started = false;
    return ESP_OK;
}

esp_err_t esp_wifi_connect(void)
{
    s_connect_count++;
    return ESP_OK;
}

esp_err_t esp_wifi_disconnect(void)
{
    s_disconnect_count++;
    return ESP_OK;
}

esp_err_t esp_wifi_set_mode(wifi_mode_t mode)
{
    s_mode = mode;
    return ESP_OK;
}

esp_err_t esp_wifi_get_mode(wifi_mode_t *mode)
{
    if (mode == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *mode = s_mode;
    return ESP_OK;
}

esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf)
{
    if (conf == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (interface == WIFI_IF_STA)
    {
        s_sta_cfg = *conf;
    }
    else if (interface == WIFI_IF_AP)
    {
        s_ap_cfg = *conf;
    }
    return ESP_OK;
}

esp_err_t esp_wifi_get_config(wifi_interface_t interface, wifi_config_t *conf)
{
    if (conf == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (interface == WIFI_IF_STA)
    {
        *conf = s_sta_cfg;
    }
    else if (interface == WIFI_IF_AP)
    {
        *conf = s_ap_cfg;
    }
    return ESP_OK;
}

esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, int block)
{
    (void)config;
    (void)block;
    return ESP_OK;
}

esp_err_t esp_wifi_scan_get_ap_num(uint16_t *number)
{
    if (number == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *number = s_scan_count;
    return ESP_OK;
}

esp_err_t esp_wifi_scan_get_ap_records(uint16_t *number, wifi_ap_record_t *ap_records)
{
    if (number == NULL || ap_records == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t to_copy = (*number < s_scan_count) ? *number : s_scan_count;
    memcpy(ap_records, s_scan_results, to_copy * sizeof(wifi_ap_record_t));
    *number = to_copy;
    return ESP_OK;
}

esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info)
{
    if (ap_info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_ap_info_set)
    {
        return ESP_ERR_NOT_FOUND;
    }
    *ap_info = s_ap_info;
    return ESP_OK;
}

esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6])
{
    (void)ifx;
    mac[0] = 0xAA;
    mac[1] = 0xBB;
    mac[2] = 0xCC;
    mac[3] = 0xDD;
    mac[4] = 0xEE;
    mac[5] = 0xFF;
    return ESP_OK;
}

esp_err_t esp_wifi_deauth_sta(uint16_t aid)
{
    (void)aid;
    return ESP_OK;
}
