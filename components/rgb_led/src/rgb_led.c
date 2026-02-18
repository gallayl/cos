#include "rgb_led.h"
#include "cyd_rgb_led_pins.h"

#include "driver/ledc.h"
#include "esp_log.h"
#include "nvs.h"

static const char *const TAG = "rgb_led";

#define RGB_LED_NVS_NAMESPACE "rgb_led"
#define RGB_LED_NVS_KEY "color"
#define RGB_LED_COLOR_BLOB_LEN 3

static uint8_t s_r = 0;
static uint8_t s_g = 0;
static uint8_t s_b = 0;

void rgb_led_register_commands(void);

/* --- Hardware --- */

static void hw_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode = CYD_RGB_LED_LEDC_SPEED,
        .duty_resolution = CYD_RGB_LED_LEDC_DUTY_RES,
        .timer_num = CYD_RGB_LED_LEDC_TIMER,
        .freq_hz = CYD_RGB_LED_LEDC_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_conf);

    const int pins[] = {CYD_RGB_LED_PIN_R, CYD_RGB_LED_PIN_G, CYD_RGB_LED_PIN_B};
    const ledc_channel_t channels[] = {CYD_RGB_LED_LEDC_CH_R, CYD_RGB_LED_LEDC_CH_G, CYD_RGB_LED_LEDC_CH_B};

    for (int i = 0; i < 3; i++)
    {
        ledc_channel_config_t ch = {
            .gpio_num = pins[i],
            .speed_mode = CYD_RGB_LED_LEDC_SPEED,
            .channel = channels[i],
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = CYD_RGB_LED_LEDC_TIMER,
            .duty = 255, // active-low: 255 = LED off
            .hpoint = 0,
        };
        ledc_channel_config(&ch);
    }
}

static void hw_apply(uint8_t r, uint8_t g, uint8_t b)
{
    ledc_set_duty(CYD_RGB_LED_LEDC_SPEED, CYD_RGB_LED_LEDC_CH_R, 255 - r);
    ledc_update_duty(CYD_RGB_LED_LEDC_SPEED, CYD_RGB_LED_LEDC_CH_R);
    ledc_set_duty(CYD_RGB_LED_LEDC_SPEED, CYD_RGB_LED_LEDC_CH_G, 255 - g);
    ledc_update_duty(CYD_RGB_LED_LEDC_SPEED, CYD_RGB_LED_LEDC_CH_G);
    ledc_set_duty(CYD_RGB_LED_LEDC_SPEED, CYD_RGB_LED_LEDC_CH_B, 255 - b);
    ledc_update_duty(CYD_RGB_LED_LEDC_SPEED, CYD_RGB_LED_LEDC_CH_B);
}

/* --- NVS persistence --- */

static esp_err_t nvs_save_color(uint8_t r, uint8_t g, uint8_t b)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(RGB_LED_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t blob[RGB_LED_COLOR_BLOB_LEN] = {r, g, b};
    err = nvs_set_blob(handle, RGB_LED_NVS_KEY, blob, sizeof(blob));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write color: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

static esp_err_t nvs_load_color(uint8_t *r, uint8_t *g, uint8_t *b)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(RGB_LED_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        return err;
    }

    uint8_t blob[RGB_LED_COLOR_BLOB_LEN];
    size_t len = sizeof(blob);
    err = nvs_get_blob(handle, RGB_LED_NVS_KEY, blob, &len);
    nvs_close(handle);

    if (err == ESP_OK && len == RGB_LED_COLOR_BLOB_LEN)
    {
        *r = blob[0];
        *g = blob[1];
        *b = blob[2];
    }

    return err;
}

/* --- Public API --- */

esp_err_t rgb_led_init(void)
{
    hw_init();

    uint8_t r = 0, g = 0, b = 0;
    esp_err_t err = nvs_load_color(&r, &g, &b);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "No saved color, defaulting to off");
    }
    else if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to load saved color: %s", esp_err_to_name(err));
    }

    s_r = r;
    s_g = g;
    s_b = b;
    hw_apply(s_r, s_g, s_b);

    rgb_led_register_commands();

    ESP_LOGI(TAG, "RGB LED initialized (color %u,%u,%u)", s_r, s_g, s_b);
    return ESP_OK;
}

esp_err_t rgb_led_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    s_r = r;
    s_g = g;
    s_b = b;
    hw_apply(s_r, s_g, s_b);
    return nvs_save_color(s_r, s_g, s_b);
}

void rgb_led_get_color(uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (r)
    {
        *r = s_r;
    }
    if (g)
    {
        *g = s_g;
    }
    if (b)
    {
        *b = s_b;
    }
}

esp_err_t rgb_led_off(void)
{
    return rgb_led_set_color(0, 0, 0);
}
