#include "unity.h"
#include "rgb_led.h"
#include "mock_ledc.h"
#include "mock_nvs.h"

void setUp(void)
{
    mock_nvs_reset();
    mock_ledc_reset();
}

void tearDown(void)
{
}

/* --- rgb_led_set_color / rgb_led_get_color --- */

void test_set_and_get_color(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_init());

    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_set_color(100, 150, 200));

    uint8_t r, g, b;
    rgb_led_get_color(&r, &g, &b);
    TEST_ASSERT_EQUAL_UINT8(100, r);
    TEST_ASSERT_EQUAL_UINT8(150, g);
    TEST_ASSERT_EQUAL_UINT8(200, b);
}

void test_get_color_null_pointers(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_init());
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_set_color(10, 20, 30));

    uint8_t r;
    rgb_led_get_color(&r, NULL, NULL);
    TEST_ASSERT_EQUAL_UINT8(10, r);
}

void test_off_sets_zero(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_init());
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_set_color(255, 255, 255));
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_off());

    uint8_t r, g, b;
    rgb_led_get_color(&r, &g, &b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);
}

/* --- NVS persistence --- */

void test_color_persists_to_nvs(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_init());
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_set_color(42, 84, 126));

    /* Simulate re-init by calling init again (re-loads from NVS) */
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_init());

    uint8_t r, g, b;
    rgb_led_get_color(&r, &g, &b);
    TEST_ASSERT_EQUAL_UINT8(42, r);
    TEST_ASSERT_EQUAL_UINT8(84, g);
    TEST_ASSERT_EQUAL_UINT8(126, b);
}

void test_init_defaults_to_off_when_no_nvs_data(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_init());

    uint8_t r, g, b;
    rgb_led_get_color(&r, &g, &b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);
}

void test_off_persists_to_nvs(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_init());
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_set_color(255, 128, 64));
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_off());

    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_init());

    uint8_t r, g, b;
    rgb_led_get_color(&r, &g, &b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);
}

void test_nvs_failure_on_save_returns_error(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_init());
    mock_nvs_set_next_error(ESP_FAIL);
    TEST_ASSERT_EQUAL(ESP_FAIL, rgb_led_set_color(1, 2, 3));
}

/* --- LEDC duty (active-low) --- */

void test_ledc_duty_active_low(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_init());
    TEST_ASSERT_EQUAL(ESP_OK, rgb_led_set_color(100, 0, 255));

    TEST_ASSERT_EQUAL_UINT32(155, mock_ledc_get_duty(0)); /* 255 - 100 */
    TEST_ASSERT_EQUAL_UINT32(255, mock_ledc_get_duty(1)); /* 255 - 0 */
    TEST_ASSERT_EQUAL_UINT32(0, mock_ledc_get_duty(2));   /* 255 - 255 */
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_set_and_get_color);
    RUN_TEST(test_get_color_null_pointers);
    RUN_TEST(test_off_sets_zero);

    RUN_TEST(test_color_persists_to_nvs);
    RUN_TEST(test_init_defaults_to_off_when_no_nvs_data);
    RUN_TEST(test_off_persists_to_nvs);
    RUN_TEST(test_nvs_failure_on_save_returns_error);

    RUN_TEST(test_ledc_duty_active_low);

    return UNITY_END();
}
