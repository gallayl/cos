#include "unity.h"
#include "http_server.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_valid_flash_path(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_sanitize_upload_path("/flash/data/file.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/flash/data/file.txt", out);
}

void test_valid_sdcard_path(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_sanitize_upload_path("/sdcard/file.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/sdcard/file.txt", out);
}

void test_rejects_dotdot(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path("../etc/passwd", out, sizeof(out)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path("/flash/../flash/file.txt", out, sizeof(out)));
}

void test_normalizes_double_slashes(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_sanitize_upload_path("//flash//data//file.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/flash/data/file.txt", out);
}

void test_rejects_empty_path(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path("", out, sizeof(out)));
}

void test_rejects_null(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path(NULL, out, sizeof(out)));
}

void test_rejects_no_prefix(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path("/data/file.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path("file.txt", out, sizeof(out)));
}

void test_strips_trailing_slash(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_sanitize_upload_path("/flash/data/", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/flash/data", out);
}

void test_backslash_normalized(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_sanitize_upload_path("/flash\\data\\file.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/flash/data/file.txt", out);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_flash_path);
    RUN_TEST(test_valid_sdcard_path);
    RUN_TEST(test_rejects_dotdot);
    RUN_TEST(test_normalizes_double_slashes);
    RUN_TEST(test_rejects_empty_path);
    RUN_TEST(test_rejects_null);
    RUN_TEST(test_rejects_no_prefix);
    RUN_TEST(test_strips_trailing_slash);
    RUN_TEST(test_backslash_normalized);
    return UNITY_END();
}
