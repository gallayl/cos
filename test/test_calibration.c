#include "unity.h"
#include "calibration.h"
#include "calibration_storage.h"
#include "mock_nvs.h"
#include "mock_display.h"

#include <string.h>

static const uint16_t MOCK_CAL[DISPLAY_CAL_DATA_LEN] = {111, 222, 333, 444, 555, 666, 777, 888};

void setUp(void)
{
    mock_nvs_reset();
    mock_display_reset();
}

void tearDown(void)
{
}

/* --- calibration_run --- */

void test_run_performs_calibration_and_saves(void)
{
    mock_display_set_cal_result(MOCK_CAL);

    TEST_ASSERT_EQUAL(ESP_OK, calibration_run());

    TEST_ASSERT_EQUAL(1, mock_display_calibrate_call_count());
    TEST_ASSERT_EQUAL(1, mock_display_set_cal_call_count());
    TEST_ASSERT_EQUAL(1, mock_display_fill_screen_call_count());

    TEST_ASSERT_EQUAL_UINT16_ARRAY(MOCK_CAL, mock_display_get_applied_cal(), DISPLAY_CAL_DATA_LEN);

    TEST_ASSERT_TRUE(calibration_storage_exists(0));
}

void test_run_saves_for_current_rotation(void)
{
    mock_display_set_rotation(3);
    mock_display_set_cal_result(MOCK_CAL);

    TEST_ASSERT_EQUAL(ESP_OK, calibration_run());

    TEST_ASSERT_TRUE(calibration_storage_exists(3));
    TEST_ASSERT_FALSE(calibration_storage_exists(0));
}

void test_run_saves_data_matches_load(void)
{
    mock_display_set_cal_result(MOCK_CAL);

    TEST_ASSERT_EQUAL(ESP_OK, calibration_run());

    uint16_t loaded[DISPLAY_CAL_DATA_LEN] = {0};
    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_load(0, loaded));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(MOCK_CAL, loaded, DISPLAY_CAL_DATA_LEN);
}

/* --- calibration_load --- */

void test_load_applies_stored_calibration(void)
{
    calibration_storage_save(0, MOCK_CAL);

    TEST_ASSERT_EQUAL(ESP_OK, calibration_load());
    TEST_ASSERT_EQUAL(1, mock_display_set_cal_call_count());
    TEST_ASSERT_EQUAL_UINT16_ARRAY(MOCK_CAL, mock_display_get_applied_cal(), DISPLAY_CAL_DATA_LEN);
}

void test_load_returns_not_found_when_no_data(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, calibration_load());
    TEST_ASSERT_EQUAL(0, mock_display_set_cal_call_count());
}

void test_load_uses_current_rotation(void)
{
    mock_display_set_rotation(2);
    calibration_storage_save(2, MOCK_CAL);

    TEST_ASSERT_EQUAL(ESP_OK, calibration_load());
    TEST_ASSERT_EQUAL_UINT16_ARRAY(MOCK_CAL, mock_display_get_applied_cal(), DISPLAY_CAL_DATA_LEN);
}

void test_load_fails_for_wrong_rotation(void)
{
    mock_display_set_rotation(0);
    calibration_storage_save(1, MOCK_CAL);

    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, calibration_load());
}

/* --- calibration_init --- */

void test_init_loads_existing_calibration(void)
{
    calibration_storage_save(0, MOCK_CAL);

    TEST_ASSERT_EQUAL(ESP_OK, calibration_init());
    TEST_ASSERT_EQUAL(1, mock_display_set_cal_call_count());
}

void test_init_succeeds_with_no_stored_data(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, calibration_init());
    TEST_ASSERT_EQUAL(0, mock_display_set_cal_call_count());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_run_performs_calibration_and_saves);
    RUN_TEST(test_run_saves_for_current_rotation);
    RUN_TEST(test_run_saves_data_matches_load);

    RUN_TEST(test_load_applies_stored_calibration);
    RUN_TEST(test_load_returns_not_found_when_no_data);
    RUN_TEST(test_load_uses_current_rotation);
    RUN_TEST(test_load_fails_for_wrong_rotation);

    RUN_TEST(test_init_loads_existing_calibration);
    RUN_TEST(test_init_succeeds_with_no_stored_data);

    return UNITY_END();
}
