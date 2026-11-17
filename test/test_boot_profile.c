#include "unity.h"

#include "boot_profile.h"
#include "mock_nvs.h"
#include "nvs.h"

#include <string.h>

void setUp(void)
{
    mock_nvs_reset();
    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_early_init());
}

void tearDown(void) {}

static void test_parse_valid_and_invalid_names(void)
{
    boot_profile_t profile = BOOT_PROFILE_DEFAULT;

    TEST_ASSERT_TRUE(boot_profile_parse("default", &profile));
    TEST_ASSERT_EQUAL(BOOT_PROFILE_DEFAULT, profile);

    TEST_ASSERT_TRUE(boot_profile_parse("low-memory-offline", &profile));
    TEST_ASSERT_EQUAL(BOOT_PROFILE_LOW_MEMORY_OFFLINE, profile);

    TEST_ASSERT_FALSE(boot_profile_parse("DEFAULT", &profile));
    TEST_ASSERT_FALSE(boot_profile_parse("low_memory_offline", &profile));
    TEST_ASSERT_FALSE(boot_profile_parse("bogus", &profile));
    TEST_ASSERT_FALSE(boot_profile_parse("", &profile));
    TEST_ASSERT_FALSE(boot_profile_parse(NULL, &profile));
}

static void test_early_init_no_stored_value_uses_default_fallback(void)
{
    char stored[40] = {0};
    bool has_stored = true;

    TEST_ASSERT_EQUAL(BOOT_PROFILE_DEFAULT, boot_profile_get_effective());
    TEST_ASSERT_EQUAL(BOOT_PROFILE_DEFAULT, boot_profile_get_next());
    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_get_stored(stored, sizeof(stored), &has_stored));
    TEST_ASSERT_EQUAL_STRING("default", stored);
    TEST_ASSERT_FALSE(has_stored);
}

static void test_early_init_valid_stored_low_memory_offline(void)
{
    /* Simulate stored value before boot read. */
    mock_nvs_reset();
    {
        nvs_handle_t handle = 0;
        TEST_ASSERT_EQUAL(ESP_OK, nvs_open("boot", NVS_READWRITE, &handle));
        TEST_ASSERT_EQUAL(ESP_OK, nvs_set_str(handle, "profile", "low-memory-offline"));
        TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(handle));
        nvs_close(handle);
    }

    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_early_init());
    TEST_ASSERT_EQUAL(BOOT_PROFILE_LOW_MEMORY_OFFLINE, boot_profile_get_effective());
    TEST_ASSERT_EQUAL(BOOT_PROFILE_LOW_MEMORY_OFFLINE, boot_profile_get_next());

    char stored[40] = {0};
    bool has_stored = false;
    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_get_stored(stored, sizeof(stored), &has_stored));
    TEST_ASSERT_EQUAL_STRING("low-memory-offline", stored);
    TEST_ASSERT_TRUE(has_stored);
}

static void test_early_init_invalid_stored_value_falls_back_to_default(void)
{
    mock_nvs_reset();
    {
        nvs_handle_t handle = 0;
        TEST_ASSERT_EQUAL(ESP_OK, nvs_open("boot", NVS_READWRITE, &handle));
        TEST_ASSERT_EQUAL(ESP_OK, nvs_set_str(handle, "profile", "bogus"));
        TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(handle));
        nvs_close(handle);
    }

    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_early_init());
    TEST_ASSERT_EQUAL(BOOT_PROFILE_DEFAULT, boot_profile_get_effective());
    TEST_ASSERT_EQUAL(BOOT_PROFILE_DEFAULT, boot_profile_get_next());

    char stored[40] = {0};
    bool has_stored = true;
    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_get_stored(stored, sizeof(stored), &has_stored));
    TEST_ASSERT_EQUAL_STRING("default", stored);
    TEST_ASSERT_FALSE(has_stored);
}

static void test_set_stored_updates_stored_and_next_but_not_effective(void)
{
    /* Start from default effective boot. */
    TEST_ASSERT_EQUAL(BOOT_PROFILE_DEFAULT, boot_profile_get_effective());

    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_set_stored("low-memory-offline"));
    TEST_ASSERT_EQUAL(BOOT_PROFILE_DEFAULT, boot_profile_get_effective());
    TEST_ASSERT_EQUAL(BOOT_PROFILE_LOW_MEMORY_OFFLINE, boot_profile_get_next());

    char stored[40] = {0};
    bool has_stored = false;
    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_get_stored(stored, sizeof(stored), &has_stored));
    TEST_ASSERT_EQUAL_STRING("low-memory-offline", stored);
    TEST_ASSERT_TRUE(has_stored);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, boot_profile_set_stored("bogus"));
    /* Invalid set must not mutate stored selection. */
    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_get_stored(stored, sizeof(stored), &has_stored));
    TEST_ASSERT_EQUAL_STRING("low-memory-offline", stored);
    TEST_ASSERT_TRUE(has_stored);
}

static void test_clear_stored_resets_next_to_default_without_changing_effective(void)
{
    /* Effective low-memory-offline for current boot, then clear stored selection. */
    mock_nvs_reset();
    {
        nvs_handle_t handle = 0;
        TEST_ASSERT_EQUAL(ESP_OK, nvs_open("boot", NVS_READWRITE, &handle));
        TEST_ASSERT_EQUAL(ESP_OK, nvs_set_str(handle, "profile", "low-memory-offline"));
        TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(handle));
        nvs_close(handle);
    }
    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_early_init());
    TEST_ASSERT_EQUAL(BOOT_PROFILE_LOW_MEMORY_OFFLINE, boot_profile_get_effective());

    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_clear_stored());
    TEST_ASSERT_EQUAL(BOOT_PROFILE_LOW_MEMORY_OFFLINE, boot_profile_get_effective());
    TEST_ASSERT_EQUAL(BOOT_PROFILE_DEFAULT, boot_profile_get_next());

    char stored[40] = {0};
    bool has_stored = true;
    TEST_ASSERT_EQUAL(ESP_OK, boot_profile_get_stored(stored, sizeof(stored), &has_stored));
    TEST_ASSERT_EQUAL_STRING("default", stored);
    TEST_ASSERT_FALSE(has_stored);
}

static void test_get_stored_reports_buffer_too_small(void)
{
    char tiny[4] = {0};
    bool has_stored = false;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, boot_profile_get_stored(tiny, sizeof(tiny), &has_stored));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_valid_and_invalid_names);
    RUN_TEST(test_early_init_no_stored_value_uses_default_fallback);
    RUN_TEST(test_early_init_valid_stored_low_memory_offline);
    RUN_TEST(test_early_init_invalid_stored_value_falls_back_to_default);
    RUN_TEST(test_set_stored_updates_stored_and_next_but_not_effective);
    RUN_TEST(test_clear_stored_resets_next_to_default_without_changing_effective);
    RUN_TEST(test_get_stored_reports_buffer_too_small);
    return UNITY_END();
}
