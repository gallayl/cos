#include "bluetooth.h"
#include "brightness.h"
#include "calibration.h"
#include "display.h"
#include "filesystem.h"
#include "http_server.h"
#include "i2c_bus.h"
#include "light_sensor.h"
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
    ESP_ERROR_CHECK(bluetooth_init());
    bluetooth_hid_set_keyboard_callback(text_console_input);
    ESP_ERROR_CHECK(time_sync_init());
    ESP_ERROR_CHECK(http_server_init());
    ESP_ERROR_CHECK(websocket_init());
    ESP_ERROR_CHECK(system_init());
    ESP_ERROR_CHECK(shell_init());

    ESP_LOGI(TAG, "COS ready");
}
