#include "unity.h"

#include "calibration_storage.h"
#include "display.h"
#include "nvs_flash.h"

#include <string.h>

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void fill_test_data(uint16_t *data, uint16_t seed)
{
    for (int i = 0; i < DISPLAY_CAL_DATA_LEN; i++)
    {
        data[i] = seed + (uint16_t)i;
    }
}

TEST_CASE("save and load round-trip", "[nvs_cal]")
{
    uint16_t saved[DISPLAY_CAL_DATA_LEN];
    uint16_t loaded[DISPLAY_CAL_DATA_LEN];

    fill_test_data(saved, 1000);
    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_save(0, saved));
    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_load(0, loaded));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(saved, loaded, DISPLAY_CAL_DATA_LEN);
}

TEST_CASE("overwrite existing calibration", "[nvs_cal]")
{
    uint16_t first[DISPLAY_CAL_DATA_LEN];
    uint16_t second[DISPLAY_CAL_DATA_LEN];
    uint16_t loaded[DISPLAY_CAL_DATA_LEN];

    fill_test_data(first, 2000);
    fill_test_data(second, 3000);

    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_save(1, first));
    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_save(1, second));
    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_load(1, loaded));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(second, loaded, DISPLAY_CAL_DATA_LEN);
}

TEST_CASE("load non-existent key returns error", "[nvs_cal]")
{
    uint16_t loaded[DISPLAY_CAL_DATA_LEN];

    TEST_ASSERT_NOT_EQUAL(ESP_OK, calibration_storage_load(5, loaded));
}

TEST_CASE("multiple rotations are independent", "[nvs_cal]")
{
    uint16_t data_r0[DISPLAY_CAL_DATA_LEN];
    uint16_t data_r3[DISPLAY_CAL_DATA_LEN];
    uint16_t loaded[DISPLAY_CAL_DATA_LEN];

    fill_test_data(data_r0, 4000);
    fill_test_data(data_r3, 5000);

    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_save(0, data_r0));
    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_save(3, data_r3));

    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_load(0, loaded));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(data_r0, loaded, DISPLAY_CAL_DATA_LEN);

    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_load(3, loaded));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(data_r3, loaded, DISPLAY_CAL_DATA_LEN);
}

TEST_CASE("exists returns true after save", "[nvs_cal]")
{
    uint16_t data[DISPLAY_CAL_DATA_LEN];
    fill_test_data(data, 6000);

    TEST_ASSERT_EQUAL(ESP_OK, calibration_storage_save(2, data));
    TEST_ASSERT_TRUE(calibration_storage_exists(2));
}

TEST_CASE("exists returns false for unsaved rotation", "[nvs_cal]")
{
    TEST_ASSERT_FALSE(calibration_storage_exists(7));
}

TEST_CASE("invalid rotation rejected", "[nvs_cal]")
{
    uint16_t data[DISPLAY_CAL_DATA_LEN];
    fill_test_data(data, 7000);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, calibration_storage_save(8, data));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, calibration_storage_save(255, data));
}

TEST_CASE("null pointer rejected", "[nvs_cal]")
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, calibration_storage_save(0, NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, calibration_storage_load(0, NULL));
}

void app_main(void)
{
    init_nvs();
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
}
