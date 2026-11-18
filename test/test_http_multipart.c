#include "unity.h"

#include "esp_err.h"
#include "esp_http_server.h"
#include "mock_httpd.h"

#include <string.h>

/* Access static helpers (extract_boundary, parse_part_headers, append_field_value). */
#include "http_multipart.c"

void setUp(void)
{
    mock_httpd_reset();
}

void tearDown(void) {}

static void test_extract_boundary_unquoted(void)
{
    char ct_buf[256];
    httpd_req_t req = {0};
    mock_httpd_set_header("Content-Type", "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxk");

    char *b = extract_boundary(&req, ct_buf, sizeof(ct_buf));
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_STRING("----WebKitFormBoundary7MA4YWxk", b);
}

static void test_extract_boundary_quoted(void)
{
    char ct_buf[256];
    httpd_req_t req = {0};
    mock_httpd_set_header("Content-Type", "multipart/form-data; boundary=\"abc123\"");

    char *b = extract_boundary(&req, ct_buf, sizeof(ct_buf));
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_STRING("abc123", b);
}

static void test_extract_boundary_missing_content_type(void)
{
    char ct_buf[256];
    httpd_req_t req = {0};
    TEST_ASSERT_NULL(extract_boundary(&req, ct_buf, sizeof(ct_buf)));
}

static void test_extract_boundary_no_boundary_param(void)
{
    char ct_buf[256];
    httpd_req_t req = {0};
    mock_httpd_set_header("Content-Type", "multipart/form-data");
    TEST_ASSERT_NULL(extract_boundary(&req, ct_buf, sizeof(ct_buf)));
}

static void test_extract_boundary_unquoted_stops_at_semicolon(void)
{
    char ct_buf[256];
    httpd_req_t req = {0};
    mock_httpd_set_header("Content-Type", "multipart/form-data; boundary=xyz; charset=utf-8");

    char *b = extract_boundary(&req, ct_buf, sizeof(ct_buf));
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_STRING("xyz", b);
}

static void test_parse_part_headers_name_and_filename(void)
{
    char name[64];
    char filename[128];
    const char hdrs[] = "Content-Disposition: form-data; name=\"file\"; filename=\"hello.txt\"\r\n"
                        "Content-Type: text/plain";

    parse_part_headers(hdrs, strlen(hdrs), name, sizeof(name), filename, sizeof(filename));
    TEST_ASSERT_EQUAL_STRING("file", name);
    TEST_ASSERT_EQUAL_STRING("hello.txt", filename);
}

static void test_parse_part_headers_field_only(void)
{
    char name[64];
    char filename[128];
    const char hdrs[] = "Content-Disposition: form-data; name=\"path\"";

    parse_part_headers(hdrs, strlen(hdrs), name, sizeof(name), filename, sizeof(filename));
    TEST_ASSERT_EQUAL_STRING("path", name);
    TEST_ASSERT_EQUAL_STRING("", filename);
}

static void test_parse_part_headers_name_case_fallback(void)
{
    char name[64];
    char filename[128];
    const char hdrs[] = "Content-Disposition: form-data; Name=\"field\"";

    parse_part_headers(hdrs, strlen(hdrs), name, sizeof(name), filename, sizeof(filename));
    TEST_ASSERT_EQUAL_STRING("field", name);
    TEST_ASSERT_EQUAL_STRING("", filename);
}

static void test_parse_part_headers_truncates_long_values(void)
{
    char name[8];
    char filename[8];
    const char hdrs[] = "Content-Disposition: form-data; name=\"longfieldname\"; filename=\"longfilename.txt\"";

    parse_part_headers(hdrs, strlen(hdrs), name, sizeof(name), filename, sizeof(filename));
    TEST_ASSERT_EQUAL_STRING("longfie", name);
    TEST_ASSERT_EQUAL_STRING("longfil", filename);
}

static void test_append_field_value_accumulates_and_caps(void)
{
    char value[16] = {0};
    size_t len = 0;

    append_field_value(value, sizeof(value), &len, "abc", 3);
    TEST_ASSERT_EQUAL_STRING("abc", value);
    TEST_ASSERT_EQUAL_UINT32(3, (uint32_t)len);

    append_field_value(value, sizeof(value), &len, "defghijklmnop", 14);
    TEST_ASSERT_EQUAL_UINT32(15, (uint32_t)len);
    TEST_ASSERT_EQUAL_STRING("abcdefghijklmno", value);

    /* Further appends when full are ignored. */
    append_field_value(value, sizeof(value), &len, "ZZ", 2);
    TEST_ASSERT_EQUAL_STRING("abcdefghijklmno", value);
    TEST_ASSERT_EQUAL_UINT32(15, (uint32_t)len);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_extract_boundary_unquoted);
    RUN_TEST(test_extract_boundary_quoted);
    RUN_TEST(test_extract_boundary_missing_content_type);
    RUN_TEST(test_extract_boundary_no_boundary_param);
    RUN_TEST(test_extract_boundary_unquoted_stops_at_semicolon);
    RUN_TEST(test_parse_part_headers_name_and_filename);
    RUN_TEST(test_parse_part_headers_field_only);
    RUN_TEST(test_parse_part_headers_name_case_fallback);
    RUN_TEST(test_parse_part_headers_truncates_long_values);
    RUN_TEST(test_append_field_value_accumulates_and_caps);
    return UNITY_END();
}
