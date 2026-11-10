#include "network.h"

#include "wifi.h"

#include "esp_netif.h"
#include "esp_wifi.h"

#include <stdio.h>
#include <string.h>

static void format_ip4(const esp_ip4_addr_t *address, char *out, size_t out_size)
{
    if (address == NULL || address->addr == 0)
    {
        if (out_size > 0)
        {
            out[0] = '\0';
        }
        return;
    }
    snprintf(out, out_size, IPSTR, IP2STR(address));
}

static void format_mac(const uint8_t mac[6], char *out, size_t out_size)
{
    snprintf(out, out_size, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void copy_ssid(char *out, size_t out_size, const uint8_t *ssid)
{
    size_t length = 0;
    while (length < 32 && length + 1 < out_size && ssid[length] != 0)
    {
        ++length;
    }
    memcpy(out, ssid, length);
    out[length] = '\0';
}

static const char *mode_name(wifi_mode_t mode)
{
    switch (mode)
    {
    case WIFI_MODE_STA:
        return "STA";
    case WIFI_MODE_AP:
        return "AP";
    case WIFI_MODE_APSTA:
        return "AP+STA";
    default:
        return "NULL";
    }
}

static void add_dns(network_host_info_t *info, esp_netif_t *netif, esp_netif_dns_type_t kind)
{
    if (info->dns_count >= 2 || netif == NULL)
    {
        return;
    }

    esp_netif_dns_info_t dns = {0};
    if (esp_netif_get_dns_info(netif, kind, &dns) != ESP_OK)
    {
        return;
    }
    if (dns.ip.type != ESP_IPADDR_TYPE_V4 || dns.ip.u_addr.ip4.addr == 0)
    {
        return;
    }
    format_ip4(&dns.ip.u_addr.ip4, info->dns[info->dns_count], sizeof(info->dns[0]));
    if (info->dns[info->dns_count][0] != '\0')
    {
        ++info->dns_count;
    }
}

static void read_sta_address(char *ip, size_t ip_size, char *netmask, size_t netmask_size, char *gateway,
                             size_t gateway_size)
{
    if (ip_size > 0)
    {
        ip[0] = '\0';
    }
    if (netmask != NULL && netmask_size > 0)
    {
        netmask[0] = '\0';
    }
    if (gateway != NULL && gateway_size > 0)
    {
        gateway[0] = '\0';
    }

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL)
    {
        return;
    }
    esp_netif_ip_info_t addresses = {0};
    if (esp_netif_get_ip_info(netif, &addresses) != ESP_OK)
    {
        return;
    }
    format_ip4(&addresses.ip, ip, ip_size);
    if (netmask != NULL)
    {
        format_ip4(&addresses.netmask, netmask, netmask_size);
    }
    if (gateway != NULL)
    {
        format_ip4(&addresses.gw, gateway, gateway_size);
    }
}

esp_err_t network_get_host_info(network_host_info_t *info)
{
    if (info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(info, 0, sizeof(*info));

    uint8_t mac[6] = {0};
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) != ESP_OK)
    {
        return ESP_ERR_INVALID_STATE;
    }
    format_mac(mac, info->mac, sizeof(info->mac));
    info->connected = wifi_is_connected();
    read_sta_address(info->ip, sizeof(info->ip), info->netmask, sizeof(info->netmask), info->gateway,
                     sizeof(info->gateway));

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    add_dns(info, netif, ESP_NETIF_DNS_MAIN);
    add_dns(info, netif, ESP_NETIF_DNS_BACKUP);
    return ESP_OK;
}

esp_err_t network_get_wifi_info(network_wifi_info_t *info)
{
    if (info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(info, 0, sizeof(*info));

    wifi_mode_t mode = WIFI_MODE_NULL;
    if (esp_wifi_get_mode(&mode) != ESP_OK)
    {
        return ESP_ERR_INVALID_STATE;
    }
    snprintf(info->mode, sizeof(info->mode), "%s", mode_name(mode));

    uint8_t mac[6] = {0};
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) != ESP_OK)
    {
        return ESP_ERR_INVALID_STATE;
    }
    format_mac(mac, info->mac, sizeof(info->mac));

    wifi_config_t sta = {0};
    if (esp_wifi_get_config(WIFI_IF_STA, &sta) == ESP_OK)
    {
        copy_ssid(info->ssid, sizeof(info->ssid), sta.sta.ssid);
    }

    info->connected = wifi_is_connected();
    read_sta_address(info->ip, sizeof(info->ip), NULL, 0, NULL, 0);

    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif != NULL)
    {
        esp_netif_ip_info_t addresses = {0};
        if (esp_netif_get_ip_info(ap_netif, &addresses) == ESP_OK)
        {
            format_ip4(&addresses.ip, info->ap_ip, sizeof(info->ap_ip));
        }
    }

    if (info->connected)
    {
        wifi_ap_record_t ap = {0};
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
        {
            copy_ssid(info->ssid, sizeof(info->ssid), ap.ssid);
            info->rssi = ap.rssi;
            snprintf(info->signal, sizeof(info->signal), "%s", wifi_signal_strength_str(ap.rssi));
            snprintf(info->auth, sizeof(info->auth), "%s", wifi_auth_mode_str(ap.authmode));
        }
    }
    return ESP_OK;
}

esp_err_t network_wifi_connect(const char *ssid, const char *password)
{
    if (ssid == NULL || ssid[0] == '\0' || strlen(ssid) > 32)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (password != NULL && strlen(password) > 63)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return wifi_connect(ssid, password);
}

esp_err_t network_wifi_disconnect(void)
{
    return wifi_disconnect();
}
