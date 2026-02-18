#include "unity.h"
#include "http_server.h"
#include "mock_httpd.h"
#include "mock_nvs.h"
#include "mbedtls/base64.h"

#include <stdio.h>
#include <string.h>

static void build_basic_header(const char *user, const char *pass, char *out, size_t out_len)
{
    char plain[128];
    snprintf(plain, sizeof(plain), "%s:%s", user, pass);

    unsigned char b64[256];
    size_t b64_len = 0;
    mbedtls_base64_encode(b64, sizeof(b64), &b64_len, (const unsigned char *)plain, strlen(plain));

    snprintf(out, out_len, "Basic %s", (char *)b64);
}

void setUp(void)
{
    mock_httpd_reset();
    mock_nvs_reset();
    /* Re-init auth with default credentials */
    http_auth_set_credentials("cos", "cos12345");
}

void tearDown(void) {}

void test_valid_credentials(void)
{
    char hdr[256];
    build_basic_header("cos", "cos12345", hdr, sizeof(hdr));
    mock_httpd_set_header("Authorization", hdr);

    httpd_req_t req = {0};
    TEST_ASSERT_EQUAL(ESP_OK, http_auth_check(&req));
}

void test_wrong_password(void)
{
    char hdr[256];
    build_basic_header("cos", "wrong", hdr, sizeof(hdr));
    mock_httpd_set_header("Authorization", hdr);

    httpd_req_t req = {0};
    TEST_ASSERT_EQUAL(ESP_FAIL, http_auth_check(&req));
}

void test_wrong_username(void)
{
    char hdr[256];
    build_basic_header("admin", "cos12345", hdr, sizeof(hdr));
    mock_httpd_set_header("Authorization", hdr);

    httpd_req_t req = {0};
    TEST_ASSERT_EQUAL(ESP_FAIL, http_auth_check(&req));
}

void test_missing_header(void)
{
    httpd_req_t req = {0};
    TEST_ASSERT_EQUAL(ESP_FAIL, http_auth_check(&req));
}

void test_malformed_not_basic(void)
{
    mock_httpd_set_header("Authorization", "Bearer some-token");

    httpd_req_t req = {0};
    TEST_ASSERT_EQUAL(ESP_FAIL, http_auth_check(&req));
}

void test_malformed_no_colon(void)
{
    unsigned char b64[128];
    size_t b64_len = 0;
    mbedtls_base64_encode(b64, sizeof(b64), &b64_len,
                          (const unsigned char *)"nocolon", 7);

    char hdr[256];
    snprintf(hdr, sizeof(hdr), "Basic %s", (char *)b64);
    mock_httpd_set_header("Authorization", hdr);

    httpd_req_t req = {0};
    TEST_ASSERT_EQUAL(ESP_FAIL, http_auth_check(&req));
}

void test_credential_update(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, http_auth_set_credentials("admin", "secret"));
    TEST_ASSERT_EQUAL_STRING("admin", http_auth_get_username());

    char hdr[256];
    build_basic_header("admin", "secret", hdr, sizeof(hdr));
    mock_httpd_set_header("Authorization", hdr);

    httpd_req_t req = {0};
    TEST_ASSERT_EQUAL(ESP_OK, http_auth_check(&req));
}

void test_old_credentials_rejected_after_update(void)
{
    http_auth_set_credentials("admin", "secret");

    char hdr[256];
    build_basic_header("cos", "cos12345", hdr, sizeof(hdr));
    mock_httpd_set_header("Authorization", hdr);

    httpd_req_t req = {0};
    TEST_ASSERT_EQUAL(ESP_FAIL, http_auth_check(&req));
}

void test_invalid_credentials_rejected(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_auth_set_credentials(NULL, "pass"));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_auth_set_credentials("user", NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_auth_set_credentials("", "pass"));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, http_auth_set_credentials("user", ""));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_credentials);
    RUN_TEST(test_wrong_password);
    RUN_TEST(test_wrong_username);
    RUN_TEST(test_missing_header);
    RUN_TEST(test_malformed_not_basic);
    RUN_TEST(test_malformed_no_colon);
    RUN_TEST(test_credential_update);
    RUN_TEST(test_old_credentials_rejected_after_update);
    RUN_TEST(test_invalid_credentials_rejected);
    return UNITY_END();
}
