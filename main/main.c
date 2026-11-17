#include "berry_runtime.h"
#include "bluetooth.h"
#include "boot_profile.h"
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
#include "service_lifecycle.h"
#include "shell.h"
#include "system.h"
#include "text_console.h"
#include "time_sync.h"
#include "websocket.h"
#include "wifi.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include <string.h>

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

static bool s_nvs_ok = false;

static void log_service_result(service_descriptor_t *svc)
{
    if (svc == NULL)
    {
        return;
    }

    if (!svc->enabled)
    {
        ESP_LOGI(TAG, "Service %s: disabled", svc->name);
        return;
    }

    if (svc->state == SERVICE_LIFECYCLE_STARTED)
    {
        return;
    }

    if (svc->state == SERVICE_LIFECYCLE_FAILED && svc->last_result == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "Skipping %s init because dependencies are unavailable", svc->name);
        return;
    }

    if (svc->state == SERVICE_LIFECYCLE_FAILED)
    {
        ESP_LOGE(TAG, "%s init failed: %s", svc->name, esp_err_to_name(svc->last_result));
    }
}

static esp_err_t svc_init_display(service_descriptor_t *svc)
{
    (void)svc;
    return display_init();
}
static esp_err_t svc_init_calibration(service_descriptor_t *svc)
{
    (void)svc;
    return calibration_init();
}
static esp_err_t svc_init_light_sensor(service_descriptor_t *svc)
{
    (void)svc;
    return light_sensor_init();
}
static esp_err_t svc_init_rgb_led(service_descriptor_t *svc)
{
    (void)svc;
    return rgb_led_init();
}
static esp_err_t svc_init_brightness(service_descriptor_t *svc)
{
    (void)svc;
    return brightness_init();
}

static esp_err_t svc_init_text_console(service_descriptor_t *svc)
{
    (void)svc;
    esp_err_t err = text_console_init();
    if (err == ESP_OK && s_nvs_ok)
    {
        text_console_apply_saved_log_level();
    }
    return err;
}

static esp_err_t svc_init_i2c_bus(service_descriptor_t *svc)
{
    (void)svc;
    return i2c_bus_init();
}
static esp_err_t svc_init_filesystem(service_descriptor_t *svc)
{
    (void)svc;
    return filesystem_init();
}
static esp_err_t svc_init_wifi(service_descriptor_t *svc)
{
    (void)svc;
    return wifi_init();
}
static esp_err_t svc_init_network(service_descriptor_t *svc)
{
    (void)svc;
    return network_init();
}
static esp_err_t svc_init_bluetooth(service_descriptor_t *svc)
{
    (void)svc;
    return bluetooth_init();
}
static esp_err_t svc_init_time_sync(service_descriptor_t *svc)
{
    (void)svc;
    return time_sync_init();
}
static esp_err_t svc_init_http_server(service_descriptor_t *svc)
{
    (void)svc;
    return http_server_init();
}
static esp_err_t svc_init_websocket(service_descriptor_t *svc)
{
    (void)svc;
    return websocket_init();
}
static esp_err_t svc_init_system(service_descriptor_t *svc)
{
    (void)svc;
    return system_init();
}
static esp_err_t svc_init_shell(service_descriptor_t *svc)
{
    (void)svc;
    return shell_init();
}

static bool svc_started(const service_descriptor_t *svc)
{
    return svc != NULL && svc->enabled && svc->state == SERVICE_LIFECYCLE_STARTED;
}

void app_main(void)
{
    esp_err_t err = init_nvs();
    s_nvs_ok = (err == ESP_OK);
    if (!s_nvs_ok)
    {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
    }

    (void)boot_profile_early_init();

    ESP_LOGI(TAG, "COS starting up (boot profile: effective=%s, next=%s)",
             boot_profile_name(boot_profile_get_effective()), boot_profile_name(boot_profile_get_next()));

    const bool allow_wifi = boot_profile_allows_wifi();
    const bool allow_network_diag = boot_profile_allows_network_diagnostics();
    const bool allow_http = boot_profile_allows_http_server();
    const bool allow_ws = boot_profile_allows_websocket();
    const bool allow_time_sync = boot_profile_allows_time_sync();

    static const char *const ws_deps[] = {"http_server"};

    static service_descriptor_t services[] = {
        {.id = "display",
         .name = "Display",
         .init = svc_init_display,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_ALWAYS_ON,
         .enabled = true},
        {.id = "calibration",
         .name = "Calibration",
         .init = svc_init_calibration,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_ALWAYS_ON,
         .enabled = true},
        {.id = "light_sensor",
         .name = "Light sensor",
         .init = svc_init_light_sensor,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_ALWAYS_ON,
         .enabled = true},
        {.id = "rgb_led",
         .name = "RGB LED",
         .init = svc_init_rgb_led,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_ALWAYS_ON,
         .enabled = true},
        {.id = "brightness",
         .name = "Brightness",
         .init = svc_init_brightness,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_ALWAYS_ON,
         .enabled = true},
        {.id = "text_console",
         .name = "Text console",
         .init = svc_init_text_console,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_ALWAYS_ON,
         .enabled = true},
        {.id = "i2c_bus",
         .name = "I2C bus",
         .init = svc_init_i2c_bus,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_ALWAYS_ON,
         .enabled = true},
        {.id = "filesystem",
         .name = "Filesystem",
         .init = svc_init_filesystem,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_ALWAYS_ON,
         .enabled = true},
        {.id = "wifi", .name = "Wi-Fi", .init = svc_init_wifi, .capabilities = SERVICE_CAP_CAN_START, .enabled = true},
        {.id = "network",
         .name = "Network diagnostics",
         .init = svc_init_network,
         .capabilities = SERVICE_CAP_CAN_START,
         .enabled = true},
        {.id = "bluetooth",
         .name = "Bluetooth",
         .init = svc_init_bluetooth,
         .capabilities = SERVICE_CAP_CAN_START,
         .enabled = true},
        {.id = "time_sync",
         .name = "Time sync",
         .init = svc_init_time_sync,
         .capabilities = SERVICE_CAP_CAN_START,
         .enabled = true},
        {.id = "http_server",
         .name = "HTTP server",
         .init = svc_init_http_server,
         .capabilities = SERVICE_CAP_CAN_START,
         .enabled = true},
        {.id = "websocket",
         .name = "WebSocket",
         .depends_on = ws_deps,
         .depends_on_count = 1,
         .init = svc_init_websocket,
         .capabilities = SERVICE_CAP_CAN_START,
         .enabled = true},
        {.id = "system",
         .name = "System",
         .init = svc_init_system,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_ALWAYS_ON,
         .enabled = true},
        {.id = "shell",
         .name = "Shell",
         .init = svc_init_shell,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_ALWAYS_ON,
         .enabled = true},
    };

    for (size_t i = 0; i < sizeof(services) / sizeof(services[0]); ++i)
    {
        service_descriptor_t *svc = &services[i];
        if (strcmp(svc->id, "wifi") == 0)
        {
            svc->enabled = allow_wifi;
        }
        else if (strcmp(svc->id, "network") == 0)
        {
            svc->enabled = allow_network_diag;
        }
        else if (strcmp(svc->id, "http_server") == 0)
        {
            svc->enabled = allow_http;
        }
        else if (strcmp(svc->id, "websocket") == 0)
        {
            svc->enabled = allow_ws;
        }
        else if (strcmp(svc->id, "time_sync") == 0)
        {
            svc->enabled = allow_time_sync;
        }
    }

    const size_t service_count = sizeof(services) / sizeof(services[0]);
    service_lifecycle_boot(services, service_count);

    for (size_t i = 0; i < service_count; ++i)
    {
        log_service_result(&services[i]);
    }

    service_descriptor_t *console_svc = service_lifecycle_find(services, service_count, "text_console");
    service_descriptor_t *fs_svc = service_lifecycle_find(services, service_count, "filesystem");
    service_descriptor_t *bt_svc = service_lifecycle_find(services, service_count, "bluetooth");
    service_descriptor_t *ws_svc = service_lifecycle_find(services, service_count, "websocket");
    service_descriptor_t *shell_svc = service_lifecycle_find(services, service_count, "shell");

    const bool console_ok = svc_started(console_svc);
    const bool fs_ok = svc_started(fs_svc);
    const bool bt_ok = svc_started(bt_svc);
    const bool ws_ok = svc_started(ws_svc);
    const bool shell_ok = svc_started(shell_svc);

    /* Best-effort: register recovery command when console is available, even if shell service failed. */
    (void)boot_profile_init();

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

    if (!boot_profile_allows_bluetooth_stack() && bt_ok)
    {
        ESP_LOGI(TAG, "Bluetooth initialized in forced-off profile mode (stack/radio unavailable this boot)");
    }

    if (fs_ok)
    {
        ESP_LOGI(TAG, "COS ready (filesystem available, profile=%s)", boot_profile_name(boot_profile_get_effective()));
    }
    else
    {
        ESP_LOGW(TAG, "COS ready with limited functionality (filesystem unavailable, profile=%s)",
                 boot_profile_name(boot_profile_get_effective()));
    }
}
