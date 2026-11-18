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

typedef struct
{
    char field_name[64];
    char field_value[256];
    char file_field_name[64];
    char file_name[128];
    char file_data[256];
    size_t file_len;
    bool file_first_seen;
    bool file_final_seen;
    int field_calls;
    int file_calls;
} parse_capture_t;

static void capture_field_cb(const char *name, const char *value, void *ctx)
{
    parse_capture_t *cap = (parse_capture_t *)ctx;
    cap->field_calls++;
    strncpy(cap->field_name, name ? name : "", sizeof(cap->field_name) - 1);
    cap->field_name[sizeof(cap->field_name) - 1] = '\0';
    strncpy(cap->field_value, value ? value : "", sizeof(cap->field_value) - 1);
    cap->field_value[sizeof(cap->field_value) - 1] = '\0';
}

static bool capture_file_cb(const char *field_name, const char *file_name, const uint8_t *data, size_t len,
                            bool is_first, bool is_final, void *ctx)
{
    parse_capture_t *cap = (parse_capture_t *)ctx;
    cap->file_calls++;
    if (is_first)
    {
        cap->file_first_seen = true;
        cap->file_len = 0;
        strncpy(cap->file_field_name, field_name ? field_name : "", sizeof(cap->file_field_name) - 1);
        cap->file_field_name[sizeof(cap->file_field_name) - 1] = '\0';
        strncpy(cap->file_name, file_name ? file_name : "", sizeof(cap->file_name) - 1);
        cap->file_name[sizeof(cap->file_name) - 1] = '\0';
    }
    if (data && len > 0 && cap->file_len + len < sizeof(cap->file_data))
    {
        memcpy(cap->file_data + cap->file_len, data, len);
        cap->file_len += len;
        cap->file_data[cap->file_len] = '\0';
    }
    if (is_final)
    {
        cap->file_final_seen = true;
    }
    return true;
}

static void test_parse_simple_file_upload(void)
{
    const char body[] = "--BOUNDARY\r\n"
                        "Content-Disposition: form-data; name=\"path\"\r\n"
                        "\r\n"
                        "/flash/test.txt\r\n"
                        "--BOUNDARY\r\n"
                        "Content-Disposition: form-data; name=\"file\"; filename=\"a.txt\"\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\n"
                        "hello\r\n"
                        "--BOUNDARY--\r\n";
    httpd_req_t req = {0};
    parse_capture_t cap = {0};

    mock_httpd_set_header("Content-Type", "multipart/form-data; boundary=BOUNDARY");
    req.content_len = strlen(body);
    mock_httpd_set_recv_body((const uint8_t *)body, req.content_len);

    esp_err_t err = http_multipart_parse(&req, capture_field_cb, capture_file_cb, &cap);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL_STRING("path", cap.field_name);
    TEST_ASSERT_EQUAL_STRING("/flash/test.txt", cap.field_value);
    TEST_ASSERT_EQUAL_STRING("file", cap.file_field_name);
    TEST_ASSERT_EQUAL_STRING("a.txt", cap.file_name);
    TEST_ASSERT_TRUE(cap.file_first_seen);
    TEST_ASSERT_TRUE(cap.file_final_seen);
    TEST_ASSERT_EQUAL_UINT32(5, (uint32_t)cap.file_len);
    TEST_ASSERT_EQUAL_STRING_LEN("hello", cap.file_data, 5);
}

static void test_parse_file_only_upload(void)
{
    const char body[] = "--B\r\n"
                        "Content-Disposition: form-data; name=\"file\"; filename=\"cfg.json\"\r\n"
                        "\r\n"
                        "{\"a\":1}\r\n"
                        "--B--\r\n";
    httpd_req_t req = {0};
    parse_capture_t cap = {0};

    mock_httpd_set_header("Content-Type", "multipart/form-data; boundary=B");
    req.content_len = strlen(body);
    mock_httpd_set_recv_body((const uint8_t *)body, req.content_len);

    esp_err_t err = http_multipart_parse(&req, NULL, capture_file_cb, &cap);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(cap.file_final_seen);
    TEST_ASSERT_EQUAL_STRING("cfg.json", cap.file_name);
    TEST_ASSERT_EQUAL_UINT32(7, (uint32_t)cap.file_len);
    TEST_ASSERT_EQUAL_STRING_LEN("{\"a\":1}", cap.file_data, 7);
}

static void test_parse_missing_boundary_returns_invalid_arg(void)
{
    const char body[] = "garbage";
    httpd_req_t req = {0};

    mock_httpd_set_header("Content-Type", "multipart/form-data");
    req.content_len = strlen(body);
    mock_httpd_set_recv_body((const uint8_t *)body, req.content_len);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_multipart_parse(&req, NULL, NULL, NULL));
}

static void test_parse_truncated_headers_returns_invalid_state(void)
{
    const char body[] = "--BOUNDARY\r\n"
                        "Content-Disposition: form-data; name=\"file\"; filename=\"a.txt\""; /* missing header terminator */
    httpd_req_t req = {0};
    parse_capture_t cap = {0};

    mock_httpd_set_header("Content-Type", "multipart/form-data; boundary=BOUNDARY");
    req.content_len = strlen(body);
    mock_httpd_set_recv_body((const uint8_t *)body, req.content_len);

    esp_err_t err = http_multipart_parse(&req, capture_field_cb, capture_file_cb, &cap);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
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
    RUN_TEST(test_parse_simple_file_upload);
    RUN_TEST(test_parse_file_only_upload);
    RUN_TEST(test_parse_missing_boundary_returns_invalid_arg);
    RUN_TEST(test_parse_truncated_headers_returns_invalid_state);
    return UNITY_END();
}
