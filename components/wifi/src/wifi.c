#include "wifi.h"
#include "wifi_default_config.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <string.h>

static const char *const TAG = "wifi";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_GOT_IP_BIT BIT1

static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;
static EventGroupHandle_t s_event_group = NULL;

void wifi_register_commands(void);

/* --- Event handler --- */

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "STA connected to AP");
            xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "STA disconnected");
            xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT | WIFI_GOT_IP_BIT);
            break;
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_event_group, WIFI_GOT_IP_BIT);
    }
}

/* --- Public API --- */

esp_err_t wifi_init(void)
{
    s_event_group = xEventGroupCreate();
    if (s_event_group == NULL)
    {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif init failed");

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "event loop create failed: %s", esp_err_to_name(err));
        return err;
    }

    s_sta_netif = esp_netif_create_default_wifi_sta();
    s_ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "esp_wifi_init failed");

    esp_event_handler_instance_t wifi_inst;
    esp_event_handler_instance_t ip_inst;
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &wifi_inst), TAG,
        "wifi event register failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &ip_inst), TAG,
        "ip event register failed");

    /* Configure AP */
    wifi_config_t ap_cfg = {};
    strncpy((char *)ap_cfg.ap.ssid, WIFI_AP_DEFAULT_SSID, sizeof(ap_cfg.ap.ssid) - 1);
    strncpy((char *)ap_cfg.ap.password, WIFI_AP_DEFAULT_PASSWORD, sizeof(ap_cfg.ap.password) - 1);
    ap_cfg.ap.ssid_len = (uint8_t)strlen(WIFI_AP_DEFAULT_SSID);
    ap_cfg.ap.max_connection = WIFI_AP_MAX_CONNECTIONS;
    ap_cfg.ap.authmode = strlen(WIFI_AP_DEFAULT_PASSWORD) > 0 ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    wifi_register_commands();

    if (!wifi_has_stored_credentials())
    {
        ESP_LOGI(TAG, "No stored credentials, starting AP-only mode");
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "set AP mode failed");
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg), TAG, "set AP config failed");
        ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");
        ESP_LOGI(TAG, "WiFi AP started (SSID: %s)", WIFI_AP_DEFAULT_SSID);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stored credentials found, starting AP+STA mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "set APSTA mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg), TAG, "set AP config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");
    ESP_RETURN_ON_ERROR(esp_wifi_connect(), TAG, "wifi connect failed");

    ESP_LOGI(TAG, "WiFi initialized, connecting...");
    return ESP_OK;
}

esp_err_t wifi_connect(const char *ssid, const char *password)
{
    if (ssid == NULL || ssid[0] == '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_wifi_disconnect();

    wifi_config_t sta_cfg = {};
    strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid) - 1);
    if (password != NULL)
    {
        strncpy((char *)sta_cfg.sta.password, password, sizeof(sta_cfg.sta.password) - 1);
    }
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg), TAG, "set STA config failed");

    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_AP)
    {
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "set APSTA mode failed");
    }

    ESP_LOGI(TAG, "Connecting to '%s'...", ssid);
    return esp_wifi_connect();
}

esp_err_t wifi_disconnect(void)
{
    return esp_wifi_disconnect();
}

esp_err_t wifi_scan(wifi_ap_record_t *results, uint16_t *count, uint16_t max_results)
{
    if (results == NULL || count == NULL || max_results == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_scan_config_t scan_cfg = {
        .show_hidden = true,
    };
    ESP_RETURN_ON_ERROR(esp_wifi_scan_start(&scan_cfg, true), TAG, "scan start failed");

    uint16_t ap_count = 0;
    ESP_RETURN_ON_ERROR(esp_wifi_scan_get_ap_num(&ap_count), TAG, "scan get count failed");

    *count = (ap_count < max_results) ? ap_count : max_results;
    ESP_RETURN_ON_ERROR(esp_wifi_scan_get_ap_records(count, results), TAG, "scan get records failed");

    return ESP_OK;
}

bool wifi_is_connected(void)
{
    if (s_event_group == NULL)
    {
        return false;
    }
    return (xEventGroupGetBits(s_event_group) & WIFI_GOT_IP_BIT) != 0;
}

bool wifi_has_stored_credentials(void)
{
    wifi_config_t conf = {};
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK)
    {
        return false;
    }
    return strlen((const char *)conf.sta.ssid) > 0;
}

const char *wifi_signal_strength_str(int8_t rssi)
{
    if (rssi > -30)
    {
        return "Amazing";
    }
    if (rssi > -67)
    {
        return "Very good";
    }
    if (rssi > -70)
    {
        return "Okay";
    }
    if (rssi > -80)
    {
        return "Not good";
    }
    if (rssi > -90)
    {
        return "Unusable";
    }
    return "Unknown";
}

const char *wifi_auth_mode_str(wifi_auth_mode_t mode)
{
    switch (mode)
    {
    case WIFI_AUTH_OPEN:
        return "OPEN";
    case WIFI_AUTH_WEP:
        return "WEP";
    case WIFI_AUTH_WPA_PSK:
        return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
        return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "WPA2_ENTERPRISE";
    case WIFI_AUTH_WPA3_PSK:
        return "WPA3_PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "WPA2_WPA3_PSK";
    default:
        return "UNKNOWN";
    }
}
