#include "wifi.h"

#include "esp_console.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "wifi_cmd";

#define WIFI_SCAN_MAX_RESULTS 20

static const char *mode_str(wifi_mode_t mode)
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

static void print_info(void)
{
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    printf("Mode:    %s\n", mode_str(mode));

    if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA)
    {
        esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (ap_netif != NULL)
        {
            esp_netif_ip_info_t ip_info = {};
            esp_netif_get_ip_info(ap_netif, &ip_info);
            printf("AP IP:   " IPSTR "\n", IP2STR(&ip_info.ip));
        }
    }

    if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA)
    {
        wifi_config_t sta_conf = {};
        esp_wifi_get_config(WIFI_IF_STA, &sta_conf);
        if (sta_conf.sta.ssid[0] != '\0')
        {
            printf("STA:     %s\n", (const char *)sta_conf.sta.ssid);
        }

        if (wifi_is_connected())
        {
            esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (sta_netif != NULL)
            {
                esp_netif_ip_info_t ip_info = {};
                esp_netif_get_ip_info(sta_netif, &ip_info);
                printf("STA IP:  " IPSTR "\n", IP2STR(&ip_info.ip));
            }

            wifi_ap_record_t ap_info = {};
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
            {
                printf("Signal:  %d dBm (%s)\n", ap_info.rssi, wifi_signal_strength_str(ap_info.rssi));
            }
        }
        else
        {
            printf("STA IP:  (not connected)\n");
        }
    }

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    printf("MAC:     %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static int cmd_wifi(int argc, char **argv)
{
    if (argc < 2)
    {
        print_info();
        return 0;
    }

    const char *subcmd = argv[1];

    if (strcmp(subcmd, "connect") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: wifi connect <ssid> [password]\n");
            return 1;
        }
        const char *password = (argc >= 4) ? argv[3] : NULL;
        esp_err_t err = wifi_connect(argv[2], password);
        if (err != ESP_OK)
        {
            printf("wifi: connect failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Connecting to '%s'...\n", argv[2]);
        return 0;
    }

    if (strcmp(subcmd, "disconnect") == 0)
    {
        esp_err_t err = wifi_disconnect();
        if (err != ESP_OK)
        {
            printf("wifi: disconnect failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Disconnected\n");
        return 0;
    }

    if (strcmp(subcmd, "scan") == 0)
    {
        wifi_ap_record_t results[WIFI_SCAN_MAX_RESULTS];
        uint16_t count = 0;
        esp_err_t err = wifi_scan(results, &count, WIFI_SCAN_MAX_RESULTS);
        if (err != ESP_OK)
        {
            printf("wifi: scan failed (%s)\n", esp_err_to_name(err));
            return 1;
        }

        printf("%-24s %5s  %-12s %s\n", "SSID", "RSSI", "Signal", "Auth");
        for (uint16_t i = 0; i < count; i++)
        {
            printf("%-24s %5d  %-12s %s\n", (const char *)results[i].ssid, results[i].rssi,
                   wifi_signal_strength_str(results[i].rssi), wifi_auth_mode_str(results[i].authmode));
        }
        printf("%u network(s) found\n", count);
        return 0;
    }

    if (strcmp(subcmd, "restart") == 0)
    {
        esp_wifi_disconnect();
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK)
        {
            printf("wifi: restart failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("Reconnecting...\n");
        return 0;
    }

    printf("Usage: wifi [connect <ssid> [pw] | disconnect | scan | restart]\n");
    return 1;
}

void wifi_register_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "wifi",
        .help = "WiFi management (info, connect, disconnect, scan, restart)",
        .hint = "[connect <ssid> [pw] | disconnect | scan | restart]",
        .func = &cmd_wifi,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(TAG, "Registered 'wifi' command");
}
