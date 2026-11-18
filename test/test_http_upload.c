#include "http_server.h"
#include "unity.h"

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

void test_url_decode_keeps_plain_path(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_url_decode("/flash/app.ts", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/flash/app.ts", out);
}

void test_url_decode_percent_encoded_path(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_url_decode("%2Fflash%2Fmy%20file.ts", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/flash/my file.ts", out);
}

void test_url_decode_plus_is_space(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_url_decode("/flash/my+file.ts", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/flash/my file.ts", out);
}

void test_url_decode_rejects_bad_escape(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_url_decode("%2", out, sizeof(out)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_url_decode("%zz", out, sizeof(out)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_url_decode("%00", out, sizeof(out)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_url_decode(NULL, out, sizeof(out)));
}

void test_url_decode_rejects_short_buffer(void)
{
    char out[4];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, http_url_decode("/flash", out, sizeof(out)));
}

void test_backslash_normalized(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_sanitize_upload_path("/flash\\data\\file.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/flash/data/file.txt", out);
}

void test_sanitize_root_paths(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_sanitize_upload_path("/flash", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/flash", out);
    TEST_ASSERT_EQUAL(ESP_OK, http_sanitize_upload_path("/sdcard", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/sdcard", out);
    TEST_ASSERT_EQUAL(ESP_OK, http_sanitize_upload_path("/flash/", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/flash", out);
}

void test_sanitize_rejects_prefix_only_partial(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path("/fla", out, sizeof(out)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path("/sd", out, sizeof(out)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path("/other/file", out, sizeof(out)));
}

void test_sanitize_invalid_args(void)
{
    char out[8];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path("/flash/a", NULL, sizeof(out)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_sanitize_upload_path("/flash/a", out, 0));
}

void test_url_decode_mixed_case_hex_and_encoded_slash(void)
{
    char out[128];
    TEST_ASSERT_EQUAL(ESP_OK, http_url_decode("%2fFlAsH%2Fa%2Eb", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("/FlAsH/a.b", out);
}

void test_url_decode_exact_fit_and_null_dst(void)
{
    char out[4];
    TEST_ASSERT_EQUAL(ESP_OK, http_url_decode("abc", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("abc", out);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_url_decode("abc", NULL, sizeof(out)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_url_decode("abc", out, 0));
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
    RUN_TEST(test_sanitize_root_paths);
    RUN_TEST(test_sanitize_rejects_prefix_only_partial);
    RUN_TEST(test_sanitize_invalid_args);
    RUN_TEST(test_url_decode_keeps_plain_path);
    RUN_TEST(test_url_decode_percent_encoded_path);
    RUN_TEST(test_url_decode_plus_is_space);
    RUN_TEST(test_url_decode_mixed_case_hex_and_encoded_slash);
    RUN_TEST(test_url_decode_exact_fit_and_null_dst);
    RUN_TEST(test_url_decode_rejects_bad_escape);
    RUN_TEST(test_url_decode_rejects_short_buffer);
    return UNITY_END();
}
