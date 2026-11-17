#include "wifi.h"
#include "wifi_default_config.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs.h"

#include <string.h>

#ifndef ESP_ERR_NOT_ALLOWED
#define ESP_ERR_NOT_ALLOWED ESP_ERR_INVALID_STATE
#endif
#ifndef ESP_ERR_WIFI_NOT_INIT
#define ESP_ERR_WIFI_NOT_INIT ESP_ERR_INVALID_STATE
#endif
#ifndef ESP_ERR_WIFI_NOT_STARTED
#define ESP_ERR_WIFI_NOT_STARTED ESP_ERR_INVALID_STATE
#endif
#ifndef ESP_ERR_WIFI_NOT_CONNECT
#define ESP_ERR_WIFI_NOT_CONNECT ESP_ERR_INVALID_STATE
#endif
#ifndef ESP_ERR_WIFI_CONN
#define ESP_ERR_WIFI_CONN ESP_FAIL
#endif

static const char *const TAG = "wifi";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_GOT_IP_BIT BIT1

#define WIFI_NVS_NAMESPACE "wifi"
#define WIFI_NVS_KEY_ENABLED "enabled"
#define BOOT_NVS_NAMESPACE "boot"
#define BOOT_NVS_KEY_PROFILE "profile"
#define BOOT_PROFILE_LOW_MEM_OFFLINE "low-memory-offline"

static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;
static EventGroupHandle_t s_event_group = NULL;
static esp_event_handler_instance_t s_wifi_event_inst = NULL;
static esp_event_handler_instance_t s_ip_event_inst = NULL;

static bool s_driver_initialized = false;
static bool s_commands_registered = false;
static bool s_user_enabled = true;
static bool s_forced_off_by_profile = false;
static wifi_service_state_t s_service_state = WIFI_SERVICE_STOPPED;

void wifi_register_commands(void);

static void wifi_set_default_ap_config(wifi_config_t *ap_cfg)
{
    memset(ap_cfg, 0, sizeof(*ap_cfg));
    strncpy((char *)ap_cfg->ap.ssid, WIFI_AP_DEFAULT_SSID, sizeof(ap_cfg->ap.ssid) - 1);
    strncpy((char *)ap_cfg->ap.password, WIFI_AP_DEFAULT_PASSWORD, sizeof(ap_cfg->ap.password) - 1);
    ap_cfg->ap.ssid_len = (uint8_t)strlen(WIFI_AP_DEFAULT_SSID);
    ap_cfg->ap.max_connection = WIFI_AP_MAX_CONNECTIONS;
    ap_cfg->ap.authmode = strlen(WIFI_AP_DEFAULT_PASSWORD) > 0 ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
}

static bool wifi_read_persisted_user_enabled_state(void)
{
    bool enabled = true; /* preserve boot auto-start default when unset */
    nvs_handle_t h = 0;
    if (nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK)
    {
        uint8_t value = 1;
        size_t len = sizeof(value);
        if (nvs_get_blob(h, WIFI_NVS_KEY_ENABLED, &value, &len) == ESP_OK && len == sizeof(value))
        {
            enabled = (value != 0);
        }
        nvs_close(h);
    }
    return enabled;
}

static void wifi_load_user_enabled_state(void)
{
    s_user_enabled = wifi_read_persisted_user_enabled_state();
}

static esp_err_t wifi_write_persisted_user_enabled_state(bool enabled)
{
    nvs_handle_t h = 0;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK)
    {
        return err;
    }
    uint8_t value = enabled ? 1 : 0; // NOLINT(readability-implicit-bool-conversion)
    err = nvs_set_blob(h, WIFI_NVS_KEY_ENABLED, &value, sizeof(value));
    if (err == ESP_OK)
    {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}

static esp_err_t wifi_persist_user_enabled_state(bool enabled)
{
    esp_err_t err = wifi_write_persisted_user_enabled_state(enabled);
    if (err == ESP_OK)
    {
        s_user_enabled = enabled;
    }
    return err;
}

static void wifi_load_profile_force_state(void)
{
    s_forced_off_by_profile = false;
    nvs_handle_t h = 0;
    if (nvs_open(BOOT_NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK)
    {
        char profile[32] = {0};
        size_t len = sizeof(profile);
        if (nvs_get_blob(h, BOOT_NVS_KEY_PROFILE, profile, &len) == ESP_OK && len > 0)
        {
            if (len >= sizeof(profile))
            {
                len = sizeof(profile) - 1;
            }
            profile[len] = '\0';
            /* Support string-stored values (with/without trailing NUL in blob) and exact value. */
            if (strcmp(profile, BOOT_PROFILE_LOW_MEM_OFFLINE) == 0)
            {
                s_forced_off_by_profile = true;
            }
            else if (len > 0 && profile[len - 1] == '\0' && strcmp(profile, BOOT_PROFILE_LOW_MEM_OFFLINE) == 0)
            {
                s_forced_off_by_profile = true;
            }
        }
        nvs_close(h);
    }
}

static bool wifi_should_start_for_boot(void)
{
    if (s_forced_off_by_profile)
    {
        return false;
    }
    return s_user_enabled;
}

static const char *wifi_state_reason_str(void)
{
    if (s_forced_off_by_profile)
    {
        return "forced off by boot profile";
    }
    if (!s_user_enabled)
    {
        return "user disabled (persisted)";
    }
    return "allowed";
}

static esp_err_t wifi_ensure_not_stopped(const char *op)
{
    (void)op;
    if (!wifi_is_service_started())
    {
        ESP_LOGW(TAG, "%s rejected: Wi-Fi service is not started (%s)", op, wifi_state_reason_str());
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (s_event_group == NULL)
    {
        return;
    }

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "STA connected to AP");
            xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
#ifdef WIFI_EVENT_STA_STOP
        case WIFI_EVENT_STA_STOP:
#endif
            ESP_LOGI(TAG, "STA disconnected/stopped");
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

static esp_err_t wifi_start_radio_locked(void)
{
    wifi_config_t ap_cfg;
    wifi_set_default_ap_config(&ap_cfg);

    if (s_event_group != NULL)
    {
        xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT | WIFI_GOT_IP_BIT);
    }

    if (!wifi_has_stored_credentials())
    {
        ESP_LOGI(TAG, "No stored credentials, starting AP-only mode");
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "set AP mode failed");
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg), TAG, "set AP config failed");
        ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");
        ESP_LOGI(TAG, "WiFi AP started (SSID: %s)", WIFI_AP_DEFAULT_SSID);
        s_service_state = WIFI_SERVICE_STARTED;
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stored credentials found, starting AP+STA mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "set APSTA mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg), TAG, "set AP config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");

    /* Radio/AP started successfully: mark service started even if STA connect initiation fails. */
    s_service_state = WIFI_SERVICE_STARTED;

    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN)
    {
        ESP_LOGW(TAG, "STA connect initiation failed (%s); service remains started (AP available)",
                 esp_err_to_name(err));
        return ESP_OK;
    }

    ESP_LOGI(TAG, "WiFi service started, connecting STA...");
    return ESP_OK;
}

static esp_err_t wifi_stop_radio_locked(void)
{
    if (!s_driver_initialized)
    {
        s_service_state = WIFI_SERVICE_STOPPED;
        return ESP_OK;
    }

    /* Best-effort stop: disconnect/radio down; keep driver/netifs/event loop for reuse. */
    esp_err_t derr = esp_wifi_disconnect();
    if (derr != ESP_OK && derr != ESP_ERR_WIFI_NOT_INIT && derr != ESP_ERR_WIFI_NOT_STARTED &&
        derr != ESP_ERR_WIFI_NOT_CONNECT)
    {
        ESP_LOGW(TAG, "wifi disconnect during stop: %s", esp_err_to_name(derr));
    }

    esp_err_t serr = esp_wifi_stop();
    if (serr != ESP_OK && serr != ESP_ERR_WIFI_NOT_INIT && serr != ESP_ERR_WIFI_NOT_STARTED)
    {
        ESP_LOGW(TAG, "wifi stop failed: %s", esp_err_to_name(serr));
        return serr;
    }

    if (s_event_group != NULL)
    {
        xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT | WIFI_GOT_IP_BIT);
    }
    s_service_state = WIFI_SERVICE_STOPPED;
    ESP_LOGI(TAG, "WiFi service stopped (best-effort, driver retained)");
    return ESP_OK;
}

static void wifi_cleanup_driver_init_failure(bool wifi_inited)
{
    if (s_wifi_event_inst != NULL)
    {
        (void)esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, s_wifi_event_inst);
        s_wifi_event_inst = NULL;
    }
    if (s_ip_event_inst != NULL)
    {
        (void)esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, s_ip_event_inst);
        s_ip_event_inst = NULL;
    }

    if (wifi_inited)
    {
        /* Best-effort cleanup on partial init failure; ignore errors to leave a known state. */
        (void)esp_wifi_stop();
        (void)esp_wifi_deinit();
    }

    /*
     * esp_netif_create_default_wifi_{sta,ap} / esp_netif_init / default event loop are
     * system-level/shared resources without a safe, complete rollback path here on ESP-IDF.
     * Clear local ownership/state so retries do not use half-initialized driver state.
     */
    s_sta_netif = NULL;
    s_ap_netif = NULL;

    if (s_event_group != NULL)
    {
        vEventGroupDelete(s_event_group);
        s_event_group = NULL;
    }

    s_driver_initialized = false;
    s_service_state = WIFI_SERVICE_STOPPED;
}

static esp_err_t wifi_ensure_driver_initialized(void)
{
    if (s_driver_initialized)
    {
        return ESP_OK;
    }

    bool wifi_inited = false;

    if (s_event_group == NULL)
    {
        s_event_group = xEventGroupCreate();
        if (s_event_group == NULL)
        {
            ESP_LOGE(TAG, "Failed to create event group");
            return ESP_ERR_NO_MEM;
        }
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "netif init failed: %s", esp_err_to_name(err));
        wifi_cleanup_driver_init_failure(wifi_inited);
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "event loop create failed: %s", esp_err_to_name(err));
        wifi_cleanup_driver_init_failure(wifi_inited);
        return err;
    }

    if (s_sta_netif == NULL)
    {
        s_sta_netif = esp_netif_create_default_wifi_sta();
    }
    if (s_ap_netif == NULL)
    {
        s_ap_netif = esp_netif_create_default_wifi_ap();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
        wifi_cleanup_driver_init_failure(wifi_inited);
        return err;
    }
    wifi_inited = true;

    if (s_wifi_event_inst == NULL)
    {
        err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,
                                                  &s_wifi_event_inst);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "wifi event register failed: %s", esp_err_to_name(err));
            wifi_cleanup_driver_init_failure(wifi_inited);
            return err;
        }
    }
    if (s_ip_event_inst == NULL)
    {
        err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL,
                                                  &s_ip_event_inst);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "ip event register failed: %s", esp_err_to_name(err));
            wifi_cleanup_driver_init_failure(wifi_inited);
            return err;
        }
    }

    s_driver_initialized = true;
    return ESP_OK;
}

esp_err_t wifi_init(void)
{
    esp_err_t err = wifi_ensure_driver_initialized();
    if (err != ESP_OK)
    {
        return err;
    }

    if (!s_commands_registered)
    {
        wifi_register_commands();
        s_commands_registered = true;
    }

    wifi_load_profile_force_state();
    wifi_load_user_enabled_state();

    if (!wifi_should_start_for_boot())
    {
        err = wifi_stop_radio_locked(); /* ensure no unintended radio/AP/STA bring-up */
        if (err != ESP_OK)
        {
            ESP_LOGW(TAG, "Failed to keep Wi-Fi stopped at boot: %s", esp_err_to_name(err));
        }
        if (s_forced_off_by_profile)
        {
            ESP_LOGI(TAG, "Wi-Fi forced off by boot profile; radio not started");
        }
        else
        {
            ESP_LOGI(TAG, "Wi-Fi disabled by persisted user setting; radio not started");
        }
        return ESP_OK;
    }

    /* Preserve default boot UX by auto-starting when allowed/enabled. */
    if (wifi_is_service_started())
    {
        return ESP_OK;
    }

    err = wifi_start_radio_locked();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Wi-Fi boot start failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t wifi_shutdown(void)
{
    if (!s_driver_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Best-effort managed shutdown: stop radio/service and keep driver resources reusable. */
    esp_err_t err = wifi_stop_radio_locked();
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Wi-Fi shutdown stop failed: %s", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
}

esp_err_t wifi_start(void)
{
    if (s_forced_off_by_profile)
    {
        ESP_LOGW(TAG, "wifi start denied: forced off by boot profile for this boot");
        return ESP_ERR_NOT_ALLOWED;
    }

    if (wifi_is_service_started())
    {
        /* Keep persisted intent aligned with an already-active service. */
        return wifi_persist_user_enabled_state(true);
    }

    const bool prev_enabled = wifi_read_persisted_user_enabled_state();
    esp_err_t perr = wifi_persist_user_enabled_state(true);
    if (perr != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to persist Wi-Fi enabled state: %s", esp_err_to_name(perr));
        return perr;
    }

    esp_err_t err = wifi_ensure_driver_initialized();
    if (err != ESP_OK)
    {
        (void)wifi_write_persisted_user_enabled_state(prev_enabled);
        s_user_enabled = prev_enabled;
        return err;
    }

    if (!s_commands_registered)
    {
        wifi_register_commands();
        s_commands_registered = true;
    }

    err = wifi_start_radio_locked();
    if (err != ESP_OK)
    {
        (void)wifi_write_persisted_user_enabled_state(prev_enabled);
        s_user_enabled = prev_enabled;
    }
    return err;
}

esp_err_t wifi_stop(void)
{
    if (s_forced_off_by_profile)
    {
        ESP_LOGW(TAG, "wifi stop denied: forced off by boot profile for this boot (persisted intent unchanged)");
        return ESP_ERR_NOT_ALLOWED;
    }

    if (!wifi_is_service_started())
    {
        /* Keep persisted intent aligned with an already-stopped service. */
        return wifi_persist_user_enabled_state(false);
    }

    const bool prev_enabled = wifi_read_persisted_user_enabled_state();
    esp_err_t perr = wifi_persist_user_enabled_state(false);
    if (perr != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to persist Wi-Fi disabled state: %s", esp_err_to_name(perr));
        return perr;
    }

    esp_err_t err = wifi_stop_radio_locked();
    if (err != ESP_OK)
    {
        (void)wifi_write_persisted_user_enabled_state(prev_enabled);
        s_user_enabled = prev_enabled;
    }
    return err;
}

esp_err_t wifi_restart(void)
{
    esp_err_t err = wifi_ensure_not_stopped("restart");
    if (err != ESP_OK)
    {
        return err;
    }

    /* Active-only restart: cycle service while preserving persisted enabled intent. */
    err = wifi_stop_radio_locked();
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "wifi restart stop phase failed: %s", esp_err_to_name(err));
        return err;
    }

    err = wifi_start_radio_locked();
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "wifi restart start phase failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Wi-Fi service restarted");
    return ESP_OK;
}

wifi_service_state_t wifi_get_service_state(void)
{
    return s_service_state;
}

bool wifi_is_service_started(void)
{
    return s_service_state == WIFI_SERVICE_STARTED;
}

bool wifi_is_forced_off_by_profile(void)
{
    return s_forced_off_by_profile;
}

bool wifi_is_user_enabled(void)
{
    return s_user_enabled;
}

esp_err_t wifi_connect(const char *ssid, const char *password)
{
    esp_err_t err = wifi_ensure_not_stopped("connect");
    if (err != ESP_OK)
    {
        return err;
    }

    if (ssid == NULL || ssid[0] == '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }

    (void)esp_wifi_disconnect();

    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_AP)
    {
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "set APSTA mode failed");
    }

    wifi_config_t sta_cfg = {};
    strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid) - 1);
    if (password != NULL)
    {
        strncpy((char *)sta_cfg.sta.password, password, sizeof(sta_cfg.sta.password) - 1);
    }
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg), TAG, "set STA config failed");

    ESP_LOGI(TAG, "Connecting to '%s'...", ssid);
    return esp_wifi_connect();
}

esp_err_t wifi_disconnect(void)
{
    esp_err_t err = wifi_ensure_not_stopped("disconnect");
    if (err != ESP_OK)
    {
        return err;
    }
    return esp_wifi_disconnect();
}

esp_err_t wifi_scan(wifi_ap_record_t *results, uint16_t *count, uint16_t max_results)
{
    esp_err_t err = wifi_ensure_not_stopped("scan");
    if (err != ESP_OK)
    {
        return err;
    }

    if (results == NULL || count == NULL || max_results == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_mode_t mode = WIFI_MODE_NULL;
    bool switched_to_apsta = false;
    ESP_RETURN_ON_ERROR(esp_wifi_get_mode(&mode), TAG, "get wifi mode failed");
    if (mode == WIFI_MODE_AP)
    {
        /* Scanning typically requires STA; temporarily enable AP+STA while service is active. */
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "set APSTA mode for scan failed");
        switched_to_apsta = true;
    }

    wifi_scan_config_t scan_cfg = {
        .show_hidden = true,
    };
    err = esp_wifi_scan_start(&scan_cfg, true);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "scan start failed: %s", esp_err_to_name(err));
        if (switched_to_apsta)
        {
            (void)esp_wifi_set_mode(WIFI_MODE_AP);
        }
        return err;
    }

    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "scan get count failed: %s", esp_err_to_name(err));
        if (switched_to_apsta)
        {
            (void)esp_wifi_set_mode(WIFI_MODE_AP);
        }
        return err;
    }

    *count = (ap_count < max_results) ? ap_count : max_results;
    err = esp_wifi_scan_get_ap_records(count, results);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "scan get records failed: %s", esp_err_to_name(err));
    }

    if (switched_to_apsta)
    {
        esp_err_t restore_err = esp_wifi_set_mode(WIFI_MODE_AP);
        if (restore_err != ESP_OK)
        {
            ESP_LOGW(TAG, "restore AP mode after scan failed: %s", esp_err_to_name(restore_err));
            if (err == ESP_OK)
            {
                err = restore_err;
            }
        }
    }

    return err;
}

bool wifi_is_connected(void)
{
    if (s_event_group == NULL || !wifi_is_service_started())
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

void wifi_test_reset(void)
{
    s_sta_netif = NULL;
    s_ap_netif = NULL;
    s_event_group = NULL;
    s_wifi_event_inst = NULL;
    s_ip_event_inst = NULL;
    s_driver_initialized = false;
    s_commands_registered = false;
    s_user_enabled = true;
    s_forced_off_by_profile = false;
    s_service_state = WIFI_SERVICE_STOPPED;
}
