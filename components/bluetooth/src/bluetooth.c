#include "bluetooth.h"
#include "bluetooth_hid.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_check.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "esp_hid_common.h"
#include "esp_hidh.h"
#include "esp_hidh_gattc.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "bluetooth";

#define BT_SCAN_DURATION_S 10
#define BT_SCAN_MAX_RESULTS 20
#define BT_SCAN_TIMEOUT_MS 15000
#define BT_CLOSE_TIMEOUT_MS 3000
#define BT_ENABLE_TASK_STACK 8192
#define BT_ENABLE_TASK_PRIO 1
#define BT_MAX_DEV_NAME_LEN 64

void bluetooth_register_commands(void);

/* ── Scan result storage ─────────────────────────────────────────────── */

typedef struct
{
    esp_bd_addr_t bda;
    uint8_t addr_type;
    char name[BT_MAX_DEV_NAME_LEN + 1];
    int8_t rssi;
    uint16_t appearance;
} bt_scan_result_t;

static bt_scan_result_t s_scan_results[BT_SCAN_MAX_RESULTS];
static int s_scan_count;
static SemaphoreHandle_t s_scan_done;
static SemaphoreHandle_t s_scan_params_set;

/* ── State ───────────────────────────────────────────────────────────── */

static volatile bool s_enabled;
static volatile bool s_enable_in_progress;
static volatile bool s_hidh_inited;
static esp_hidh_dev_t *volatile s_hid_dev;
static SemaphoreHandle_t s_close_done;

/* ── NVS persistence ─────────────────────────────────────────────────── */

#define NVS_NAMESPACE "bluetooth"
#define NVS_KEY_ENABLED "enabled"
#define NVS_KEY_AUTO_BDA "auto_bda"
#define NVS_KEY_AUTO_ADDR_TYPE "auto_atype"

static void nvs_save_enabled(bool enabled)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_set_u8(h, NVS_KEY_ENABLED, enabled ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}

static bool nvs_load_enabled(void)
{
    nvs_handle_t h;
    uint8_t val = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK)
    {
        nvs_get_u8(h, NVS_KEY_ENABLED, &val);
        nvs_close(h);
    }
    return val != 0;
}

static void nvs_save_auto_device(const uint8_t *bda, uint8_t addr_type)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_set_blob(h, NVS_KEY_AUTO_BDA, bda, sizeof(esp_bd_addr_t));
        nvs_set_u8(h, NVS_KEY_AUTO_ADDR_TYPE, addr_type);
        nvs_commit(h);
        nvs_close(h);
    }
}

static bool nvs_load_auto_device(esp_bd_addr_t bda_out, uint8_t *addr_type_out)
{
    nvs_handle_t h;
    size_t len = sizeof(esp_bd_addr_t);
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK)
    {
        esp_err_t err = nvs_get_blob(h, NVS_KEY_AUTO_BDA, bda_out, &len);
        if (err == ESP_OK && len == sizeof(esp_bd_addr_t))
        {
            uint8_t at = 0;
            nvs_get_u8(h, NVS_KEY_AUTO_ADDR_TYPE, &at);
            *addr_type_out = at;
            nvs_close(h);
            return true;
        }
        nvs_close(h);
    }
    return false;
}

/* ── Address-type lookup from last scan ──────────────────────────────── */

static bool lookup_addr_type(const uint8_t *bda, uint8_t *addr_type_out)
{
    for (int i = 0; i < s_scan_count; i++)
    {
        if (memcmp(s_scan_results[i].bda, bda, sizeof(esp_bd_addr_t)) == 0)
        {
            *addr_type_out = s_scan_results[i].addr_type;
            return true;
        }
    }
    return false;
}

/* ── BLE GAP callback ────────────────────────────────────────────────── */

static void ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        if (param->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGD(TAG, "BLE scan params set OK");
        }
        else
        {
            ESP_LOGW(TAG, "BLE scan params set failed: %d", param->scan_param_cmpl.status);
        }
        if (s_scan_params_set != NULL)
        {
            xSemaphoreGive(s_scan_params_set);
        }
        break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
        struct ble_scan_result_evt_param *sr = &param->scan_rst;

        if (sr->search_evt == ESP_GAP_SEARCH_INQ_RES_EVT)
        {
            if (s_scan_count >= BT_SCAN_MAX_RESULTS)
            {
                break;
            }

            uint8_t uuid_len = 0;
            uint8_t *uuid_d = esp_ble_resolve_adv_data_by_type(
                sr->ble_adv, sr->adv_data_len + sr->scan_rsp_len,
                ESP_BLE_AD_TYPE_16SRV_CMPL, &uuid_len);
            if (uuid_d == NULL || uuid_len < 2)
            {
                uuid_d = esp_ble_resolve_adv_data_by_type(
                    sr->ble_adv, sr->adv_data_len + sr->scan_rsp_len,
                    ESP_BLE_AD_TYPE_16SRV_PART, &uuid_len);
            }

            uint16_t uuid = 0;
            if (uuid_d != NULL && uuid_len >= 2)
            {
                uuid = uuid_d[0] | ((uint16_t)uuid_d[1] << 8);
            }

            if (uuid != ESP_GATT_UUID_HID_SVC)
            {
                break;
            }

            /* Skip duplicates */
            for (int i = 0; i < s_scan_count; i++)
            {
                if (memcmp(s_scan_results[i].bda, sr->bda, sizeof(esp_bd_addr_t)) == 0)
                {
                    goto done_scan_result;
                }
            }

            bt_scan_result_t *r = &s_scan_results[s_scan_count];
            memcpy(r->bda, sr->bda, sizeof(esp_bd_addr_t));
            r->addr_type = sr->ble_addr_type;
            r->rssi = (int8_t)sr->rssi;

            uint8_t app_len = 0;
            uint8_t *app_d = esp_ble_resolve_adv_data_by_type(
                sr->ble_adv, sr->adv_data_len + sr->scan_rsp_len,
                ESP_BLE_AD_TYPE_APPEARANCE, &app_len);
            r->appearance = (app_d != NULL && app_len >= 2) ? (app_d[0] | ((uint16_t)app_d[1] << 8)) : 0;

            uint8_t name_len = 0;
            uint8_t *name_d = esp_ble_resolve_adv_data_by_type(
                sr->ble_adv, sr->adv_data_len + sr->scan_rsp_len,
                ESP_BLE_AD_TYPE_NAME_CMPL, &name_len);
            if (name_d == NULL)
            {
                name_d = esp_ble_resolve_adv_data_by_type(
                    sr->ble_adv, sr->adv_data_len + sr->scan_rsp_len,
                    ESP_BLE_AD_TYPE_NAME_SHORT, &name_len);
            }
            if (name_d != NULL && name_len > 0)
            {
                size_t copy = name_len > BT_MAX_DEV_NAME_LEN ? BT_MAX_DEV_NAME_LEN : name_len;
                memcpy(r->name, name_d, copy);
                r->name[copy] = '\0';
            }
            else
            {
                r->name[0] = '\0';
            }

            s_scan_count++;
        done_scan_result:;
        }
        else if (sr->search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT)
        {
            ESP_LOGI(TAG, "BLE scan complete, %d HID device(s) found", s_scan_count);
            if (s_scan_done != NULL)
            {
                xSemaphoreGive(s_scan_done);
            }
        }
        break;
    }

    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI(TAG, "BLE security request -- accepting");
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;

    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (param->ble_security.auth_cmpl.success)
        {
            ESP_LOGI(TAG, "BLE authentication OK (addr_type=%d)", param->ble_security.auth_cmpl.addr_type);
        }
        else
        {
            ESP_LOGW(TAG, "BLE authentication FAILED (reason=0x%x)", param->ble_security.auth_cmpl.fail_reason);
        }
        break;

    case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
        if (param->remove_bond_dev_cmpl.status == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "Removed BLE bond for " ESP_BD_ADDR_STR,
                     ESP_BD_ADDR_HEX(param->remove_bond_dev_cmpl.bd_addr));
        }
        else
        {
            ESP_LOGW(TAG, "Failed to remove BLE bond (status %d)", param->remove_bond_dev_cmpl.status);
        }
        break;

    default:
        break;
    }
}

/* ── HIDH callback (esp_hid event) ───────────────────────────────────── */

static void hidh_event_cb(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;

    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *p = (esp_hidh_event_data_t *)event_data;

    switch (event)
    {
    case ESP_HIDH_OPEN_EVENT:
        if (p->open.status == ESP_OK)
        {
            const char *name = esp_hidh_dev_name_get(p->open.dev);
            const uint8_t *bda = esp_hidh_dev_bda_get(p->open.dev);
            ESP_LOGI(TAG, "HID device opened: %s (" ESP_BD_ADDR_STR ")", name ? name : "???", ESP_BD_ADDR_HEX(bda));
            s_hid_dev = p->open.dev;
            if (bda != NULL)
            {
                uint8_t at = 0;
                lookup_addr_type(bda, &at);
                nvs_save_auto_device(bda, at);
            }
        }
        else
        {
            ESP_LOGW(TAG, "HID open failed (0x%x)", p->open.status);
            if (p->open.dev != NULL)
            {
                esp_hidh_dev_free(p->open.dev);
            }
            s_hid_dev = NULL;
        }
        break;

    case ESP_HIDH_CLOSE_EVENT:
        ESP_LOGI(TAG, "HID device closed (reason=%d)", p->close.reason);
        if (p->close.dev != NULL)
        {
            esp_hidh_dev_free(p->close.dev);
        }
        s_hid_dev = NULL;
        if (s_close_done != NULL)
        {
            xSemaphoreGive(s_close_done);
        }
        break;

    case ESP_HIDH_INPUT_EVENT:
        if (p->input.usage == ESP_HID_USAGE_KEYBOARD)
        {
            bluetooth_hid_keyboard_input(p->input.data, p->input.length);
        }
        break;

    default:
        break;
    }
}

/* ── Enable sequence ─────────────────────────────────────────────────── */

static esp_err_t do_bt_enable_sequence(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Bluetooth enable started");
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "Free heap: %lu, min ever: %lu",
             (unsigned long)esp_get_free_heap_size(),
             (unsigned long)esp_get_minimum_free_heap_size());

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    err = esp_bt_controller_init(&bt_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "controller init failed: %s", esp_err_to_name(err));
        s_enable_in_progress = false;
        return err;
    }
    ESP_LOGI(TAG, "Controller init OK, enabling BLE...");

    err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "controller enable failed: %s", esp_err_to_name(err));
        esp_bt_controller_deinit();
        s_enable_in_progress = false;
        return err;
    }
    ESP_LOGI(TAG, "Controller enabled, starting Bluedroid...");

    err = esp_bluedroid_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "bluedroid init failed: %s", esp_err_to_name(err));
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        s_enable_in_progress = false;
        return err;
    }
    err = esp_bluedroid_enable();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "bluedroid enable failed: %s", esp_err_to_name(err));
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        s_enable_in_progress = false;
        return err;
    }
    ESP_LOGI(TAG, "Bluedroid enabled");

    err = esp_ble_gap_register_callback(ble_gap_cb);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "BLE GAP register failed: %s", esp_err_to_name(err));
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        s_enable_in_progress = false;
        return err;
    }

    /* BLE security: "Just Works" pairing for headless device */
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t key_size = 16;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(auth_req));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));

    ESP_LOGI(TAG, "BLE GAP OK, initializing HID host...");

    esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler);

    esp_hidh_config_t hidh_cfg = {
        .callback = hidh_event_cb,
        .event_stack_size = 4096,
        .callback_arg = NULL,
    };
    err = esp_hidh_init(&hidh_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HID host init failed: %s", esp_err_to_name(err));
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        s_enable_in_progress = false;
        return err;
    }
    s_hidh_inited = true;
    ESP_LOGI(TAG, "BLE HID host ready");

    s_enabled = true;
    s_enable_in_progress = false;
    nvs_save_enabled(true);
    ESP_LOGI(TAG, "Bluetooth enabled (BLE)");

    /* Auto-reconnect to the last paired device if stored */
    esp_bd_addr_t auto_bda;
    uint8_t auto_addr_type = 0;
    if (nvs_load_auto_device(auto_bda, &auto_addr_type))
    {
        ESP_LOGI(TAG, "Auto-connecting to " ESP_BD_ADDR_STR " (type=%d)",
                 ESP_BD_ADDR_HEX(auto_bda), auto_addr_type);
        /* Inject into scan results so bluetooth_connect can look it up */
        if (s_scan_count < BT_SCAN_MAX_RESULTS)
        {
            bt_scan_result_t *r = &s_scan_results[s_scan_count];
            memcpy(r->bda, auto_bda, sizeof(esp_bd_addr_t));
            r->addr_type = auto_addr_type;
            r->name[0] = '\0';
            r->rssi = 0;
            r->appearance = 0;
            s_scan_count++;
        }
        esp_err_t ac_err = bluetooth_connect(auto_bda);
        if (ac_err != ESP_OK)
        {
            ESP_LOGW(TAG, "Auto-connect failed: %s (device may be off)", esp_err_to_name(ac_err));
        }
    }

    return ESP_OK;
}

static void bt_enable_task(void *arg)
{
    (void)arg;
    do_bt_enable_sequence();
    vTaskDelete(NULL);
}

/* ── Public API ──────────────────────────────────────────────────────── */

esp_err_t bluetooth_init(void)
{
    s_scan_done = xSemaphoreCreateBinary();
    if (s_scan_done == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    s_scan_params_set = xSemaphoreCreateBinary();
    if (s_scan_params_set == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    s_close_done = xSemaphoreCreateBinary();
    if (s_close_done == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    bluetooth_register_commands();

    if (nvs_load_enabled())
    {
        ESP_LOGI(TAG, "NVS: Bluetooth was enabled, auto-starting...");
        bluetooth_enable();
    }

    return ESP_OK;
}

esp_err_t bluetooth_enable(void)
{
    if (s_enabled)
    {
        ESP_LOGW(TAG, "Already enabled");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_enable_in_progress)
    {
        ESP_LOGW(TAG, "Enable already in progress");
        return ESP_ERR_INVALID_STATE;
    }

    s_enable_in_progress = true;
    if (xTaskCreate(bt_enable_task, "bt_enable", BT_ENABLE_TASK_STACK, NULL, BT_ENABLE_TASK_PRIO, NULL) != pdPASS)
    {
        s_enable_in_progress = false;
        ESP_LOGE(TAG, "Failed to create enable task");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t bluetooth_disable(void)
{
    if (s_enable_in_progress)
    {
        ESP_LOGW(TAG, "Enable still in progress");
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_enabled)
    {
        ESP_LOGW(TAG, "Not enabled");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_hid_dev != NULL)
    {
        xSemaphoreTake(s_close_done, 0);
        esp_hidh_dev_close(s_hid_dev);
        if (xSemaphoreTake(s_close_done, pdMS_TO_TICKS(BT_CLOSE_TIMEOUT_MS)) != pdTRUE)
        {
            ESP_LOGW(TAG, "Timeout waiting for HID device close");
            s_hid_dev = NULL;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    if (s_hidh_inited)
    {
        esp_hidh_deinit();
        s_hidh_inited = false;
    }
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    s_enabled = false;
    nvs_save_enabled(false);
    ESP_LOGI(TAG, "Bluetooth disabled");
    return ESP_OK;
}

esp_err_t bluetooth_scan(void)
{
    if (!s_enabled)
    {
        return ESP_ERR_INVALID_STATE;
    }

    s_scan_count = 0;
    memset(s_scan_results, 0, sizeof(s_scan_results));

    xSemaphoreTake(s_scan_done, 0);
    xSemaphoreTake(s_scan_params_set, 0);

    esp_ble_scan_params_t scan_params = {
        .scan_type = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval = 0x50,
        .scan_window = 0x30,
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
    };

    ESP_RETURN_ON_ERROR(esp_ble_gap_set_scan_params(&scan_params), TAG, "set scan params failed");

    if (xSemaphoreTake(s_scan_params_set, pdMS_TO_TICKS(5000)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Timeout waiting for scan params set");
        return ESP_ERR_TIMEOUT;
    }

    printf("Scanning BLE for %d seconds...\n", BT_SCAN_DURATION_S);

    ESP_RETURN_ON_ERROR(esp_ble_gap_start_scanning(BT_SCAN_DURATION_S), TAG, "start scan failed");

    if (xSemaphoreTake(s_scan_done, pdMS_TO_TICKS(BT_SCAN_TIMEOUT_MS)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Scan timed out");
        esp_ble_gap_stop_scanning();
    }

    printf("%-24s %-18s %5s  %s\n", "Name", "Address", "RSSI", "Appearance");
    for (int i = 0; i < s_scan_count; i++)
    {
        bt_scan_result_t *r = &s_scan_results[i];
        printf("%-24s " ESP_BD_ADDR_STR " %5d  0x%04x\n",
               r->name[0] ? r->name : "(unknown)",
               ESP_BD_ADDR_HEX(r->bda), r->rssi, r->appearance);
    }
    printf("%d HID device(s) found\n", s_scan_count);

    return ESP_OK;
}

esp_err_t bluetooth_connect(const uint8_t *bda)
{
    if (!s_enabled)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_hidh_inited)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (bda == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_hid_dev != NULL)
    {
        ESP_LOGW(TAG, "Already connected -- disconnect first");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t addr_type = BLE_ADDR_TYPE_PUBLIC;
    if (!lookup_addr_type(bda, &addr_type))
    {
        ESP_LOGW(TAG, "Address type unknown for " ESP_BD_ADDR_STR " (using public)", ESP_BD_ADDR_HEX(bda));
    }

    ESP_LOGI(TAG, "Opening BLE HID connection to " ESP_BD_ADDR_STR " (type=%d)", ESP_BD_ADDR_HEX(bda), addr_type);
    esp_hidh_dev_t *dev = esp_hidh_dev_open((uint8_t *)bda, ESP_HID_TRANSPORT_BLE, addr_type);
    if (dev == NULL)
    {
        ESP_LOGE(TAG, "esp_hidh_dev_open returned NULL");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t bluetooth_disconnect(void)
{
    if (s_hid_dev == NULL)
    {
        ESP_LOGW(TAG, "No device connected");
        return ESP_ERR_INVALID_STATE;
    }
    return esp_hidh_dev_close(s_hid_dev);
}

esp_err_t bluetooth_forget(void)
{
    if (s_hid_dev != NULL)
    {
        ESP_LOGI(TAG, "Disconnecting before forget...");
        xSemaphoreTake(s_close_done, 0);
        esp_hidh_dev_close(s_hid_dev);
        xSemaphoreTake(s_close_done, pdMS_TO_TICKS(BT_CLOSE_TIMEOUT_MS));
    }

    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_erase_key(h, NVS_KEY_AUTO_BDA);
        nvs_erase_key(h, NVS_KEY_AUTO_ADDR_TYPE);
        nvs_commit(h);
        nvs_close(h);
    }

    if (s_enabled)
    {
        int dev_num = esp_ble_get_bond_device_num();
        if (dev_num > 0)
        {
            esp_ble_bond_dev_t *devs = malloc(sizeof(esp_ble_bond_dev_t) * (size_t)dev_num);
            if (devs != NULL)
            {
                if (esp_ble_get_bond_device_list(&dev_num, devs) == ESP_OK)
                {
                    for (int i = 0; i < dev_num; i++)
                    {
                        esp_ble_remove_bond_device(devs[i].bd_addr);
                    }
                }
                free(devs);
            }
            ESP_LOGI(TAG, "Removed %d BLE bonded device(s)", dev_num);
        }
    }

    ESP_LOGI(TAG, "Forgot all paired devices");
    return ESP_OK;
}

bool bluetooth_is_enabled(void)
{
    return s_enabled;
}

bool bluetooth_is_enabling(void)
{
    return s_enable_in_progress;
}

bool bluetooth_is_hid_ready(void)
{
    return s_hidh_inited;
}

bool bluetooth_is_connected(void)
{
    return s_hid_dev != NULL;
}

const char *bluetooth_connected_device_name(void)
{
    if (s_hid_dev == NULL)
    {
        return NULL;
    }
    return esp_hidh_dev_name_get(s_hid_dev);
}

const uint8_t *bluetooth_connected_device_bda(void)
{
    if (s_hid_dev == NULL)
    {
        return NULL;
    }
    return esp_hidh_dev_bda_get(s_hid_dev);
}
