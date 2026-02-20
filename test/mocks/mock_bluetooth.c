#include "mock_bluetooth.h"
#include "bluetooth.h"

#include <string.h>

static bool s_enabled;
static bool s_enabling;
static bool s_hid_ready;
static bool s_connected;
static const char *s_device_name;
static uint8_t s_device_bda[6];
static bool s_has_bda;

static esp_err_t s_enable_result;
static esp_err_t s_disable_result;
static esp_err_t s_scan_result;
static esp_err_t s_connect_result;
static esp_err_t s_disconnect_result;
static esp_err_t s_forget_result;

static int s_enable_count;
static int s_disable_count;
static int s_scan_count;
static int s_connect_count;
static int s_disconnect_count;
static int s_forget_count;
static uint8_t s_last_connect_bda[6];

void mock_bluetooth_reset(void)
{
    s_enabled = false;
    s_enabling = false;
    s_hid_ready = false;
    s_connected = false;
    s_device_name = NULL;
    s_has_bda = false;
    memset(s_device_bda, 0, sizeof(s_device_bda));

    s_enable_result = ESP_OK;
    s_disable_result = ESP_OK;
    s_scan_result = ESP_OK;
    s_connect_result = ESP_OK;
    s_disconnect_result = ESP_OK;
    s_forget_result = ESP_OK;

    s_enable_count = 0;
    s_disable_count = 0;
    s_scan_count = 0;
    s_connect_count = 0;
    s_disconnect_count = 0;
    s_forget_count = 0;
    memset(s_last_connect_bda, 0, sizeof(s_last_connect_bda));
}

void mock_bluetooth_set_enabled(bool enabled) { s_enabled = enabled; }
void mock_bluetooth_set_enabling(bool enabling) { s_enabling = enabling; }
void mock_bluetooth_set_hid_ready(bool ready) { s_hid_ready = ready; }
void mock_bluetooth_set_connected(bool connected) { s_connected = connected; }

void mock_bluetooth_set_device_name(const char *name) { s_device_name = name; }

void mock_bluetooth_set_device_bda(const uint8_t *bda)
{
    if (bda != NULL)
    {
        memcpy(s_device_bda, bda, 6);
        s_has_bda = true;
    }
    else
    {
        s_has_bda = false;
    }
}

void mock_bluetooth_set_enable_result(esp_err_t err) { s_enable_result = err; }
void mock_bluetooth_set_disable_result(esp_err_t err) { s_disable_result = err; }
void mock_bluetooth_set_scan_result(esp_err_t err) { s_scan_result = err; }
void mock_bluetooth_set_connect_result(esp_err_t err) { s_connect_result = err; }
void mock_bluetooth_set_disconnect_result(esp_err_t err) { s_disconnect_result = err; }
void mock_bluetooth_set_forget_result(esp_err_t err) { s_forget_result = err; }

int mock_bluetooth_get_enable_count(void) { return s_enable_count; }
int mock_bluetooth_get_disable_count(void) { return s_disable_count; }
int mock_bluetooth_get_scan_count(void) { return s_scan_count; }
int mock_bluetooth_get_connect_count(void) { return s_connect_count; }
const uint8_t *mock_bluetooth_get_last_connect_bda(void) { return s_last_connect_bda; }
int mock_bluetooth_get_disconnect_count(void) { return s_disconnect_count; }
int mock_bluetooth_get_forget_count(void) { return s_forget_count; }

/* --- bluetooth.h implementation (mocked) --- */

esp_err_t bluetooth_init(void)
{
    return ESP_OK;
}

esp_err_t bluetooth_enable(void)
{
    s_enable_count++;
    return s_enable_result;
}

esp_err_t bluetooth_disable(void)
{
    s_disable_count++;
    return s_disable_result;
}

esp_err_t bluetooth_scan(void)
{
    s_scan_count++;
    return s_scan_result;
}

esp_err_t bluetooth_connect(const uint8_t *bda)
{
    s_connect_count++;
    if (bda != NULL)
    {
        memcpy(s_last_connect_bda, bda, 6);
    }
    return s_connect_result;
}

esp_err_t bluetooth_disconnect(void)
{
    s_disconnect_count++;
    return s_disconnect_result;
}

esp_err_t bluetooth_forget(void)
{
    s_forget_count++;
    return s_forget_result;
}

void bluetooth_hid_set_keyboard_callback(bt_keyboard_cb_t cb)
{
    (void)cb;
}

bool bluetooth_is_enabled(void) { return s_enabled; }
bool bluetooth_is_enabling(void) { return s_enabling; }
bool bluetooth_is_hid_ready(void) { return s_hid_ready; }
bool bluetooth_is_connected(void) { return s_connected; }
const char *bluetooth_connected_device_name(void) { return s_device_name; }

const uint8_t *bluetooth_connected_device_bda(void)
{
    return s_has_bda ? s_device_bda : NULL;
}
