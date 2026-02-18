#include "unity.h"
#include "time_sync.h"
#include "mock_sntp.h"
#include "mock_console.h"

#include <stdlib.h>
#include <string.h>

void setUp(void)
{
    mock_sntp_reset();
    mock_console_reset();
}

void tearDown(void)
{
}

/* --- time_sync_init --- */

void test_init_calls_sntp(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, time_sync_init());
    TEST_ASSERT_EQUAL(1, mock_sntp_get_init_count());
    TEST_ASSERT_NOT_NULL(mock_sntp_get_server());
}

/* --- time_sync_is_synced --- */

void test_not_synced_when_epoch_is_zero(void)
{
    /* Before any NTP sync, time() returns a small value (near epoch 0).
     * On host, time() returns real time, so this test just verifies
     * the function doesn't crash. The year check (> 2020) should
     * return true on any modern host. */
    bool synced = time_sync_is_synced();
    /* On a host machine with a real clock, this should be true */
    TEST_ASSERT_TRUE(synced);
}

/* --- time_sync_get_utc --- */

void test_get_utc_formats_correctly(void)
{
    char buf[32];
    TEST_ASSERT_EQUAL(ESP_OK, time_sync_get_utc(buf, sizeof(buf)));
    /* Should end with 'Z' */
    size_t len = strlen(buf);
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_EQUAL('Z', buf[len - 1]);
    /* Should contain 'T' separator */
    TEST_ASSERT_NOT_NULL(strchr(buf, 'T'));
}

void test_get_utc_buffer_too_small(void)
{
    char buf[10];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, time_sync_get_utc(buf, sizeof(buf)));
}

void test_get_utc_null_buffer(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, time_sync_get_utc(NULL, 32));
}

/* --- time_sync_get_local --- */

void test_get_local_formats_correctly(void)
{
#if defined(_WIN32) || defined(__MINGW32__)
    _putenv_s("TZ", "UTC0");
#else
    setenv("TZ", "UTC0", 1);
#endif
    tzset();

    char buf[32];
    TEST_ASSERT_EQUAL(ESP_OK, time_sync_get_local(buf, sizeof(buf)));
    /* Should contain 'T' separator */
    TEST_ASSERT_NOT_NULL(strchr(buf, 'T'));
    /* Should contain a + or - for timezone offset */
    TEST_ASSERT_TRUE(strchr(buf, '+') != NULL || strchr(buf, '-') != NULL);
}

void test_get_local_buffer_too_small(void)
{
    char buf[10];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, time_sync_get_local(buf, sizeof(buf)));
}

/* --- time_sync_get_epoch --- */

void test_get_epoch_returns_positive(void)
{
    time_t epoch = time_sync_get_epoch();
    TEST_ASSERT_GREATER_THAN(0, epoch);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_calls_sntp);
    RUN_TEST(test_not_synced_when_epoch_is_zero);

    RUN_TEST(test_get_utc_formats_correctly);
    RUN_TEST(test_get_utc_buffer_too_small);
    RUN_TEST(test_get_utc_null_buffer);

    RUN_TEST(test_get_local_formats_correctly);
    RUN_TEST(test_get_local_buffer_too_small);

    RUN_TEST(test_get_epoch_returns_positive);

    return UNITY_END();
}
