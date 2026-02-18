#include "unity.h"
#include "brightness.h"
#include "mock_display.h"
#include "mock_esp_timer.h"
#include "mock_light_sensor.h"
#include "mock_nvs.h"
#include "nvs.h"

void setUp(void)
{
    mock_nvs_reset();
    mock_display_reset();
    mock_esp_timer_reset();
    mock_light_sensor_reset();
}

void tearDown(void)
{
}

/* --- brightness_map_adc (sensor inverted: 0 = sunshine, 4095 = dark) --- */

void test_map_adc_zero_returns_max_brightness(void)
{
    /* Full sunshine -> max screen brightness */
    TEST_ASSERT_EQUAL_UINT8(255, brightness_map_adc(0));
}

void test_map_adc_max_returns_minimum_brightness(void)
{
    /* Full darkness -> minimum screen brightness */
    TEST_ASSERT_EQUAL_UINT8(10, brightness_map_adc(4095));
}

void test_map_adc_midpoint(void)
{
    uint8_t val = brightness_map_adc(2048);
    /* (4095-2048)*255/4095 = ~127 */
    TEST_ASSERT_TRUE(val >= 126 && val <= 128);
}

void test_map_adc_negative_returns_max_brightness(void)
{
    TEST_ASSERT_EQUAL_UINT8(255, brightness_map_adc(-1));
}

void test_map_adc_above_max_returns_minimum(void)
{
    /* Beyond max darkness -> minimum brightness */
    TEST_ASSERT_EQUAL_UINT8(10, brightness_map_adc(5000));
}

void test_map_adc_high_value_clamped_to_minimum(void)
{
    /* Near-max ADC (very dark) -> near-minimum brightness, clamped to floor */
    TEST_ASSERT_EQUAL_UINT8(10, brightness_map_adc(4094));
}

/* --- brightness_set / brightness_get --- */

void test_set_and_get(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_EQUAL(ESP_OK, brightness_set(200));
    TEST_ASSERT_EQUAL_UINT8(200, brightness_get());
}

void test_set_updates_display(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_EQUAL(ESP_OK, brightness_set(42));
    TEST_ASSERT_EQUAL_UINT8(42, mock_display_get_brightness());
}

/* --- NVS persistence --- */

void test_brightness_persists(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_EQUAL(ESP_OK, brightness_set(77));

    /* Re-init loads from NVS */
    mock_esp_timer_reset();
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_EQUAL_UINT8(77, brightness_get());
}

void test_default_brightness_when_no_nvs(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_EQUAL_UINT8(128, brightness_get());
}

/* --- Auto-brightness mode --- */

void test_auto_mode_starts_timer(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_FALSE(mock_esp_timer_is_running());

    TEST_ASSERT_EQUAL(ESP_OK, brightness_set_auto(true));
    TEST_ASSERT_TRUE(brightness_is_auto());
    TEST_ASSERT_TRUE(mock_esp_timer_is_running());
}

void test_auto_mode_stops_on_manual_set(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_EQUAL(ESP_OK, brightness_set_auto(true));
    TEST_ASSERT_TRUE(mock_esp_timer_is_running());

    TEST_ASSERT_EQUAL(ESP_OK, brightness_set(100));
    TEST_ASSERT_FALSE(brightness_is_auto());
    TEST_ASSERT_FALSE(mock_esp_timer_is_running());
}

void test_auto_mode_timer_tick_adjusts_brightness(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_EQUAL(ESP_OK, brightness_set_auto(true));

    mock_light_sensor_set_value(2048);
    mock_esp_timer_fire();

    uint8_t b = brightness_get();
    TEST_ASSERT_TRUE(b >= 126 && b <= 128);
    TEST_ASSERT_EQUAL_UINT8(b, mock_display_get_brightness());
}

void test_auto_mode_persists(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_EQUAL(ESP_OK, brightness_set_auto(true));

    mock_esp_timer_reset();
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_TRUE(brightness_is_auto());
    TEST_ASSERT_TRUE(mock_esp_timer_is_running());
}

void test_disable_auto_stops_timer(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_EQUAL(ESP_OK, brightness_set_auto(true));
    TEST_ASSERT_TRUE(mock_esp_timer_is_running());

    TEST_ASSERT_EQUAL(ESP_OK, brightness_set_auto(false));
    TEST_ASSERT_FALSE(brightness_is_auto());
    TEST_ASSERT_FALSE(mock_esp_timer_is_running());
}

/* --- Rotation --- */

void test_init_applies_stored_rotation(void)
{
    /* Store rotation=2 in NVS manually */
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());

    /* Use the screen command internally -- we test via the display mock */
    mock_display_set_rotation(0);

    /* Store rotation by re-initing after setting via NVS */
    /* We can directly write NVS blob for rotation */
    {
        nvs_handle_t h;
        nvs_open("brightness", NVS_READWRITE, &h);
        uint8_t rot = 2;
        nvs_set_blob(h, "rotation", &rot, 1);
        nvs_commit(h);
        nvs_close(h);
    }

    mock_esp_timer_reset();
    TEST_ASSERT_EQUAL(ESP_OK, brightness_init());
    TEST_ASSERT_EQUAL_UINT8(2, display_get_rotation());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_map_adc_zero_returns_max_brightness);
    RUN_TEST(test_map_adc_max_returns_minimum_brightness);
    RUN_TEST(test_map_adc_midpoint);
    RUN_TEST(test_map_adc_negative_returns_max_brightness);
    RUN_TEST(test_map_adc_above_max_returns_minimum);
    RUN_TEST(test_map_adc_high_value_clamped_to_minimum);

    RUN_TEST(test_set_and_get);
    RUN_TEST(test_set_updates_display);

    RUN_TEST(test_brightness_persists);
    RUN_TEST(test_default_brightness_when_no_nvs);

    RUN_TEST(test_auto_mode_starts_timer);
    RUN_TEST(test_auto_mode_stops_on_manual_set);
    RUN_TEST(test_auto_mode_timer_tick_adjusts_brightness);
    RUN_TEST(test_auto_mode_persists);
    RUN_TEST(test_disable_auto_stops_timer);

    RUN_TEST(test_init_applies_stored_rotation);

    return UNITY_END();
}
