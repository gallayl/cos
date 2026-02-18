#include "unity.h"
#include "system.h"
#include "mock_system.h"
#include "mock_console.h"

void setUp(void)
{
    mock_system_reset();
    mock_console_reset();
}

void tearDown(void)
{
}

/* --- system_get_info --- */

void test_get_info_populates_struct(void)
{
    mock_system_set_free_heap(180000);
    mock_system_set_min_free_heap(160000);
    mock_system_set_total_heap(320000);
    mock_system_set_max_alloc(110000);
    mock_system_set_flash_size(4000000);

    system_info_t info;
    TEST_ASSERT_EQUAL(ESP_OK, system_get_info(&info));

    TEST_ASSERT_NOT_NULL(info.idf_version);
    TEST_ASSERT_EQUAL(240, info.cpu_freq_mhz);
    TEST_ASSERT_EQUAL(4000000, info.flash_size);
    TEST_ASSERT_EQUAL(180000, info.free_heap);
    TEST_ASSERT_EQUAL(160000, info.min_free_heap);
    TEST_ASSERT_EQUAL(320000, info.total_heap);
    TEST_ASSERT_EQUAL(110000, info.max_alloc_block);
}

void test_get_info_null_returns_error(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, system_get_info(NULL));
}

/* --- system_format_uptime --- */

void test_uptime_seconds_only(void)
{
    mock_system_set_uptime_us(45LL * 1000000);
    char buf[32];
    TEST_ASSERT_EQUAL(ESP_OK, system_format_uptime(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("45s", buf);
}

void test_uptime_minutes_and_seconds(void)
{
    mock_system_set_uptime_us((2LL * 60 + 30) * 1000000);
    char buf[32];
    TEST_ASSERT_EQUAL(ESP_OK, system_format_uptime(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("2m 30s", buf);
}

void test_uptime_hours_minutes_seconds(void)
{
    mock_system_set_uptime_us((1LL * 3600 + 23 * 60 + 45) * 1000000);
    char buf[32];
    TEST_ASSERT_EQUAL(ESP_OK, system_format_uptime(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("1h 23m 45s", buf);
}

void test_uptime_days(void)
{
    mock_system_set_uptime_us((2LL * 86400 + 3 * 3600 + 4 * 60 + 5) * 1000000);
    char buf[32];
    TEST_ASSERT_EQUAL(ESP_OK, system_format_uptime(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("2d 3h 4m 5s", buf);
}

void test_uptime_zero(void)
{
    mock_system_set_uptime_us(0);
    char buf[32];
    TEST_ASSERT_EQUAL(ESP_OK, system_format_uptime(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("0s", buf);
}

void test_uptime_buffer_too_small(void)
{
    mock_system_set_uptime_us(45LL * 1000000);
    char buf[2];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, system_format_uptime(buf, sizeof(buf)));
}

void test_uptime_null_buffer(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, system_format_uptime(NULL, 32));
}

/* --- system_init registers commands --- */

void test_init_registers_commands(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, system_init());

    /* Verify info command was registered and runs */
    int ret = mock_console_run_cmd("info", 1, (char *[]){"info"});
    TEST_ASSERT_EQUAL(0, ret);

    ret = mock_console_run_cmd("memory", 1, (char *[]){"memory"});
    TEST_ASSERT_EQUAL(0, ret);

    ret = mock_console_run_cmd("uptime", 1, (char *[]){"uptime"});
    TEST_ASSERT_EQUAL(0, ret);
}

/* --- system_restart --- */

void test_restart_calls_esp_restart(void)
{
    TEST_ASSERT_FALSE(mock_system_restart_called());
    system_restart();
    TEST_ASSERT_TRUE(mock_system_restart_called());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_get_info_populates_struct);
    RUN_TEST(test_get_info_null_returns_error);

    RUN_TEST(test_uptime_seconds_only);
    RUN_TEST(test_uptime_minutes_and_seconds);
    RUN_TEST(test_uptime_hours_minutes_seconds);
    RUN_TEST(test_uptime_days);
    RUN_TEST(test_uptime_zero);
    RUN_TEST(test_uptime_buffer_too_small);
    RUN_TEST(test_uptime_null_buffer);

    RUN_TEST(test_init_registers_commands);
    RUN_TEST(test_restart_calls_esp_restart);

    return UNITY_END();
}
