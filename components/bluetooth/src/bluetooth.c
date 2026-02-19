#include "bluetooth.h"
#include "bluetooth_hid.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_check.h"
#include "esp_gap_bt_api.h"
#include "esp_hid_common.h"
#include "esp_hidh.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static const char *const TAG = "bluetooth";

#define BT_SCAN_DURATION_1_28S 8 /* ~10 seconds */
#define BT_SCAN_MAX_RESULTS    20
#define BT_SCAN_TIMEOUT_MS     15000
#define BT_ENABLE_TASK_STACK   12288
#define BT_ENABLE_TASK_PRIO    1

void bluetooth_register_commands(void);

/* ── Scan result storage ─────────────────────────────────────────────── */

typedef struct {
    esp_bd_addr_t bda;
    char name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
    int8_t rssi;
    uint32_t cod;
} bt_scan_result_t;

static bt_scan_result_t s_scan_results[BT_SCAN_MAX_RESULTS];
static int s_scan_count;
static SemaphoreHandle_t s_scan_done;

/* ── State ───────────────────────────────────────────────────────────── */

static bool s_enabled;
static volatile bool s_enable_in_progress;
static volatile bool s_hidh_inited;
static esp_hidh_dev_t *s_hid_dev;

/* ── Helper: extract name from EIR ───────────────────────────────────── */

static bool get_name_from_eir(const uint8_t *eir, size_t eir_len, char *out, size_t out_size)
{
    if (eir == NULL || eir_len == 0)
    {
        return false;
    }

    size_t pos = 0;
    while (pos < eir_len)
    {
        uint8_t len = eir[pos];
        if (len == 0 || pos + 1 + len > eir_len)
        {
            break;
        }
        uint8_t type = eir[pos + 1];
        if (type == 0x09 || type == 0x08)
        {
            size_t name_len = len - 1;
            if (name_len >= out_size)
            {
                name_len = out_size - 1;
            }
            memcpy(out, &eir[pos + 2], name_len);
            out[name_len] = '\0';
            return true;
        }
        pos += 1 + len;
    }
    return false;
}

/* ── GAP callback ────────────────────────────────────────────────────── */

static void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BT_GAP_DISC_RES_EVT: {
        if (s_scan_count >= BT_SCAN_MAX_RESULTS)
        {
            break;
        }

        bt_scan_result_t *r = &s_scan_results[s_scan_count];
        memcpy(r->bda, param->disc_res.bda, sizeof(esp_bd_addr_t));
        r->name[0] = '\0';
        r->rssi = 0;
        r->cod = 0;

        for (int i = 0; i < param->disc_res.num_prop; i++)
        {
            esp_bt_gap_dev_prop_t *prop = &param->disc_res.prop[i];
            switch (prop->type)
            {
            case ESP_BT_GAP_DEV_PROP_BDNAME:
                if (prop->len > 0)
                {
                    size_t copy_len = (size_t)prop->len;
                    if (copy_len > ESP_BT_GAP_MAX_BDNAME_LEN)
                    {
                        copy_len = ESP_BT_GAP_MAX_BDNAME_LEN;
                    }
                    memcpy(r->name, prop->val, copy_len);
                    r->name[copy_len] = '\0';
                }
                break;
            case ESP_BT_GAP_DEV_PROP_RSSI:
                r->rssi = *(int8_t *)prop->val;
                break;
            case ESP_BT_GAP_DEV_PROP_COD:
                r->cod = *(uint32_t *)prop->val;
                break;
            case ESP_BT_GAP_DEV_PROP_EIR:
                if (r->name[0] == '\0')
                {
                    get_name_from_eir((uint8_t *)prop->val, (size_t)prop->len,
                                      r->name, sizeof(r->name));
                }
                break;
            default:
                break;
            }
        }
        s_scan_count++;
        break;
    }

    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED)
        {
            ESP_LOGI(TAG, "Discovery finished, %d device(s) found", s_scan_count);
            if (s_scan_done != NULL)
            {
                xSemaphoreGive(s_scan_done);
            }
        }
        else
        {
            ESP_LOGI(TAG, "Discovery started");
        }
        break;

    case ESP_BT_GAP_PIN_REQ_EVT:
        ESP_LOGI(TAG, "PIN requested by " ESP_BD_ADDR_STR " -- replying 0000",
                 ESP_BD_ADDR_HEX(param->pin_req.bda));
        {
            esp_bt_pin_code_t pin = {'0', '0', '0', '0'};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin);
        }
        break;

    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(TAG, "SSP confirm request (value: %" PRIu32 ") -- auto-accepting",
                 param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;

    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "SSP passkey notification: %" PRIu32, param->key_notif.passkey);
        printf("BT passkey: %06" PRIu32 "\n", param->key_notif.passkey);
        break;

    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGW(TAG, "SSP passkey request -- not supported, rejecting");
        esp_bt_gap_ssp_passkey_reply(param->key_req.bda, false, 0);
        break;

    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "Authentication OK with %s", param->auth_cmpl.device_name);
        }
        else
        {
            ESP_LOGW(TAG, "Authentication FAILED (status %d)", param->auth_cmpl.stat);
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
            ESP_LOGI(TAG, "HID device opened: %s (" ESP_BD_ADDR_STR ")",
                     name ? name : "???", ESP_BD_ADDR_HEX(bda));
            s_hid_dev = p->open.dev;
        }
        else
        {
            ESP_LOGW(TAG, "HID open failed (0x%x)", p->open.status);
            s_hid_dev = NULL;
        }
        break;

    case ESP_HIDH_CLOSE_EVENT:
        ESP_LOGI(TAG, "HID device closed");
        if (p->close.dev != NULL)
        {
            esp_hidh_dev_free(p->close.dev);
        }
        s_hid_dev = NULL;
        break;

    case ESP_HIDH_INPUT_EVENT:
        if (p->input.usage == ESP_HID_USAGE_KEYBOARD)
        {
            bluetooth_hid_keyboard_input(p->input.data, p->input.length);
        }
        break;

    case ESP_HIDH_START_EVENT:
        ESP_LOGI(TAG, "HID host started");
        break;

    case ESP_HIDH_STOP_EVENT:
        ESP_LOGI(TAG, "HID host stopped");
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

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    err = esp_bt_controller_init(&bt_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "controller init failed: %s", esp_err_to_name(err));
        s_enable_in_progress = false;
        return err;
    }
    ESP_LOGI(TAG, "Controller init OK, enabling...");

    err = esp_bt_controller_enable(BTDM_CONTROLLER_MODE_EFF);
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

    err = esp_bt_gap_register_callback(gap_cb);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GAP register failed: %s", esp_err_to_name(err));
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        s_enable_in_progress = false;
        return err;
    }
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(iocap));
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    ESP_LOGI(TAG, "GAP OK, initializing HID host...");

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
    ESP_LOGI(TAG, "HID host ready");

    s_enabled = true;
    s_enable_in_progress = false;
    ESP_LOGI(TAG, "Bluetooth enabled");
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

    bluetooth_register_commands();
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
    if (!s_enabled)
    {
        ESP_LOGW(TAG, "Not enabled");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_hid_dev != NULL)
    {
        esp_hidh_dev_close(s_hid_dev);
        s_hid_dev = NULL;
    }
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

    /* Reset semaphore in case of leftover signal */
    xSemaphoreTake(s_scan_done, 0);

    ESP_RETURN_ON_ERROR(
        esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, BT_SCAN_DURATION_1_28S, 0),
        TAG, "start discovery failed");

    printf("Scanning for ~%d seconds...\n", BT_SCAN_DURATION_1_28S * 128 / 100);

    if (xSemaphoreTake(s_scan_done, pdMS_TO_TICKS(BT_SCAN_TIMEOUT_MS)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Scan timed out");
        esp_bt_gap_cancel_discovery();
    }

    printf("%-24s %-18s %5s  %s\n", "Name", "Address", "RSSI", "CoD");
    for (int i = 0; i < s_scan_count; i++)
    {
        bt_scan_result_t *r = &s_scan_results[i];
        printf("%-24s " ESP_BD_ADDR_STR " %5d  0x%06" PRIx32 "\n",
               r->name[0] ? r->name : "(unknown)",
               ESP_BD_ADDR_HEX(r->bda),
               r->rssi,
               r->cod);
    }
    printf("%d device(s) found\n", s_scan_count);

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

    ESP_LOGI(TAG, "Opening HID connection to " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));
    esp_hidh_dev_t *dev = esp_hidh_dev_open((uint8_t *)bda, ESP_HID_TRANSPORT_BT, 0);
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
