#include "brightness.h"
#include "calibration.h"
#include "display.h"
#include "light_sensor.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"

static const char *const TAG = "brightness";

#define BRIGHTNESS_NVS_NAMESPACE "brightness"
#define BRIGHTNESS_NVS_KEY_LEVEL "level"
#define BRIGHTNESS_NVS_KEY_MODE "mode"
#define BRIGHTNESS_NVS_KEY_ROTATION "rotation"

#define BRIGHTNESS_DEFAULT_LEVEL 128
#define BRIGHTNESS_AUTO_MIN 10
#define BRIGHTNESS_ADC_MAX 4095
#define BRIGHTNESS_AUTO_INTERVAL_US (1000000) // 1 second

static uint8_t s_level = BRIGHTNESS_DEFAULT_LEVEL;
static uint8_t s_mode = BRIGHTNESS_MODE_MANUAL;
static uint8_t s_rotation = 0;
static esp_timer_handle_t s_auto_timer = NULL;

void brightness_register_commands(void);

/* --- NVS helpers --- */

static esp_err_t nvs_save_u8(const char *key, uint8_t value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(BRIGHTNESS_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(handle, key, &value, sizeof(value));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS write '%s' failed: %s", key, esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

static esp_err_t nvs_load_u8(const char *key, uint8_t *out)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(BRIGHTNESS_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        return err;
    }

    size_t len = sizeof(*out);
    err = nvs_get_blob(handle, key, out, &len);
    nvs_close(handle);
    return err;
}

/* --- Auto-brightness --- */

uint8_t brightness_map_adc(int adc_value)
{
    if (adc_value <= 0)
    {
        return 255;
    }
    if (adc_value >= BRIGHTNESS_ADC_MAX)
    {
        return BRIGHTNESS_AUTO_MIN;
    }

    /* Sensor is inverted: 0 = bright sunlight, 4095 = darkness */
    int mapped = ((BRIGHTNESS_ADC_MAX - adc_value) * 255) / BRIGHTNESS_ADC_MAX;
    if (mapped < BRIGHTNESS_AUTO_MIN)
    {
        mapped = BRIGHTNESS_AUTO_MIN;
    }
    return (uint8_t)mapped;
}

static void auto_brightness_cb(void *arg)
{
    (void)arg;
    int adc = light_sensor_read();
    if (adc < 0)
    {
        return;
    }
    uint8_t level = brightness_map_adc(adc);
    s_level = level;
    display_set_brightness(level);
}

static esp_err_t auto_timer_start(void)
{
    if (s_auto_timer == NULL)
    {
        const esp_timer_create_args_t args = {
            .callback = &auto_brightness_cb,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "auto_brightness",
        };
        esp_err_t err = esp_timer_create(&args, &s_auto_timer);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to create auto-brightness timer: %s", esp_err_to_name(err));
            return err;
        }
    }

    esp_err_t err = esp_timer_start_periodic(s_auto_timer, BRIGHTNESS_AUTO_INTERVAL_US);
    if (err == ESP_ERR_INVALID_STATE)
    {
        // Already running
        return ESP_OK;
    }
    return err;
}

static void auto_timer_stop(void)
{
    if (s_auto_timer != NULL)
    {
        esp_timer_stop(s_auto_timer);
    }
}

/* --- Public API --- */

esp_err_t brightness_init(void)
{
    /* Stop any running auto-brightness timer from a previous init */
    if (s_auto_timer != NULL)
    {
        esp_timer_stop(s_auto_timer);
        esp_timer_delete(s_auto_timer);
        s_auto_timer = NULL;
    }

    /* Reset to defaults, then override with stored values */
    s_level = BRIGHTNESS_DEFAULT_LEVEL;
    s_mode = BRIGHTNESS_MODE_MANUAL;
    s_rotation = 0;

    uint8_t val;
    if (nvs_load_u8(BRIGHTNESS_NVS_KEY_LEVEL, &val) == ESP_OK)
    {
        s_level = val;
    }
    if (nvs_load_u8(BRIGHTNESS_NVS_KEY_MODE, &val) == ESP_OK)
    {
        s_mode = val;
    }
    if (nvs_load_u8(BRIGHTNESS_NVS_KEY_ROTATION, &val) == ESP_OK)
    {
        s_rotation = val;
    }

    display_set_rotation(s_rotation);
    calibration_load();

    if (s_mode == BRIGHTNESS_MODE_AUTO)
    {
        auto_timer_start();
        auto_brightness_cb(NULL);
    }
    else
    {
        display_set_brightness(s_level);
    }

    brightness_register_commands();

    ESP_LOGI(TAG, "Brightness initialized (level=%u, mode=%s, rotation=%u)", s_level,
             s_mode == BRIGHTNESS_MODE_AUTO ? "auto" : "manual", s_rotation);
    return ESP_OK;
}

esp_err_t brightness_set(uint8_t value)
{
    s_level = value;
    s_mode = BRIGHTNESS_MODE_MANUAL;

    auto_timer_stop();
    display_set_brightness(s_level);

    esp_err_t err = nvs_save_u8(BRIGHTNESS_NVS_KEY_LEVEL, s_level);
    if (err != ESP_OK)
    {
        return err;
    }
    return nvs_save_u8(BRIGHTNESS_NVS_KEY_MODE, s_mode);
}

uint8_t brightness_get(void)
{
    return s_level;
}

esp_err_t brightness_set_auto(bool on)
{
    s_mode = on ? BRIGHTNESS_MODE_AUTO : BRIGHTNESS_MODE_MANUAL;

    if (on)
    {
        esp_err_t err = auto_timer_start();
        if (err != ESP_OK)
        {
            return err;
        }
        auto_brightness_cb(NULL);
    }
    else
    {
        auto_timer_stop();
    }

    return nvs_save_u8(BRIGHTNESS_NVS_KEY_MODE, s_mode);
}

bool brightness_is_auto(void)
{
    return s_mode == BRIGHTNESS_MODE_AUTO;
}
