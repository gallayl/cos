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

static const keyboard_router_t s_keyboard_router = {
    .console_owns_input = console_prompt_owns_keyboard,
    .gui_handles_event = berry_app_handles_keyboard,
    .shell_input = feed_shell_keyboard_text,
};

static void route_keyboard_event(const keyboard_event_t *event)
{
    keyboard_route_event(event, &s_keyboard_router);
}

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS partition corrupted, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

void app_main(void)
{
    ESP_LOGI(TAG, "COS starting up...");

    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(display_init());
    ESP_ERROR_CHECK(calibration_init());
    ESP_ERROR_CHECK(light_sensor_init());
    ESP_ERROR_CHECK(rgb_led_init());
    ESP_ERROR_CHECK(brightness_init());
    ESP_ERROR_CHECK(text_console_init());
    ESP_ERROR_CHECK(i2c_bus_init());
    ESP_ERROR_CHECK(filesystem_init());
    ESP_ERROR_CHECK(wifi_init());
    ESP_ERROR_CHECK(network_init());
    ESP_ERROR_CHECK(bluetooth_init());
    ESP_ERROR_CHECK(time_sync_init());
    ESP_ERROR_CHECK(http_server_init());
    ESP_ERROR_CHECK(websocket_init());
    ESP_ERROR_CHECK(system_init());
    ESP_ERROR_CHECK(shell_init());
    bluetooth_hid_set_keyboard_callback(NULL);
    bluetooth_hid_set_key_event_callback(route_keyboard_event);

    ESP_LOGI(TAG, "COS ready");
}
