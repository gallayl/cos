#include "berry_runtime.h"
#include "bluetooth.h"
#include "brightness.h"
#include "calibration.h"
#include "display.h"
#include "filesystem.h"
#include "http_server.h"
#include "i2c_bus.h"
#include "keyboard_router.h"
#include "light_sensor.h"
#include "network.h"
#include "rgb_led.h"
#include "shell.h"
#include "system.h"
#include "text_console.h"
#include "time_sync.h"
#include "websocket.h"
#include "wifi.h"

#include "esp_log.h"
#include "nvs_flash.h"

static const char *const TAG = "main";

static bool console_prompt_owns_keyboard(void *context)
{
    (void)context;
    return shell_is_reading_line_input();
}

static bool berry_app_handles_keyboard(const keyboard_event_t *event, void *context)
{
    (void)context;
    return berry_runtime_handle_key_event(event);
}

static void feed_shell_keyboard_text(const char *text, size_t length, void *context)
{
    (void)context;
    shell_feed_input(text, length);
}

static const keyboard_router_t S_KEYBOARD_ROUTER = {
    .console_owns_input = console_prompt_owns_keyboard,
    .gui_handles_event = berry_app_handles_keyboard,
    .shell_input = feed_shell_keyboard_text,
};

static void route_keyboard_event(const keyboard_event_t *event)
{
    keyboard_route_event(event, &S_KEYBOARD_ROUTER);
}

static void broadcast_console_output(const char *data, size_t len, void *context)
{
    (void)context;
    if (data == NULL || len == 0 || websocket_client_count() == 0)
    {
        return;
    }
    (void)websocket_broadcast(data, len);
}

static void submit_console_text(const char *data, size_t len, void *context)
{
    (void)context;
    shell_feed_input(data, len);
}

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS partition corrupted, erasing...");
        esp_err_t erase_err = nvs_flash_erase();
        if (erase_err != ESP_OK)
        {
            return erase_err;
        }
        err = nvs_flash_init();
    }
    return err;
}

static void log_init_result(const char *name, esp_err_t err, bool *ok_out)
{
    if (err == ESP_OK)
    {
        if (ok_out != NULL)
        {
            *ok_out = true;
        }
        return;
    }

    ESP_LOGE(TAG, "%s init failed: %s", name, esp_err_to_name(err));
    if (ok_out != NULL)
    {
        *ok_out = false;
    }
}

void app_main(void)
{
    esp_err_t err = init_nvs();
    bool nvs_ok = (err == ESP_OK);
    if (!nvs_ok)
    {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "COS starting up...");

    bool console_ok = false;
    bool fs_ok = false;
    bool bt_ok = false;
    bool http_ok = false;
    bool ws_ok = false;
    bool shell_ok = false;

    log_init_result("Display", display_init(), NULL);
    log_init_result("Calibration", calibration_init(), NULL);
    log_init_result("Light sensor", light_sensor_init(), NULL);
    log_init_result("RGB LED", rgb_led_init(), NULL);
    log_init_result("Brightness", brightness_init(), NULL);
    log_init_result("Text console", text_console_init(), &console_ok);
    if (console_ok && nvs_ok)
    {
        text_console_apply_saved_log_level();
    }
    log_init_result("I2C bus", i2c_bus_init(), NULL);
    log_init_result("Filesystem", filesystem_init(), &fs_ok);
    log_init_result("Wi-Fi", wifi_init(), NULL);
    log_init_result("Network", network_init(), NULL);
    log_init_result("Bluetooth", bluetooth_init(), &bt_ok);
    log_init_result("Time sync", time_sync_init(), NULL);

    err = http_server_init();
    log_init_result("HTTP server", err, &http_ok);
    if (http_ok)
    {
        err = websocket_init();
        log_init_result("WebSocket", err, &ws_ok);
    }
    else
    {
        ESP_LOGW(TAG, "Skipping WebSocket init because HTTP server is unavailable");
    }

    log_init_result("System", system_init(), NULL);
    log_init_result("Shell", shell_init(), &shell_ok);

    if (bt_ok && console_ok && shell_ok)
    {
        bluetooth_hid_set_keyboard_callback(NULL);
        bluetooth_hid_set_key_event_callback(route_keyboard_event);
    }
    else if (bt_ok)
    {
        ESP_LOGW(TAG, "Bluetooth keyboard routing disabled because console/shell is unavailable");
    }
    if (console_ok)
    {
        text_console_set_output_listener(broadcast_console_output, NULL);
    }
    if (ws_ok && shell_ok)
    {
        websocket_set_text_handler(submit_console_text, NULL);
    }
    else if (ws_ok)
    {
        ESP_LOGW(TAG, "WebSocket text input disabled because shell is unavailable");
    }

    if (fs_ok)
    {
        ESP_LOGI(TAG, "COS ready (filesystem available)");
    }
    else
    {
        ESP_LOGW(TAG, "COS ready with limited functionality (filesystem unavailable)");
    }
}
