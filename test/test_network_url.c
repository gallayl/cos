#include "unity.h"

#include "network.h"

#include <string.h>

void setUp(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url(""));
}

void tearDown(void) {}

static void test_absolute_http_adds_root(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("http://example.com", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://example.com/", url);
}

static void test_absolute_https_strips_default_port_and_lowercases_host(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("https://ExAmPle.COM:443/A/../b", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("https://example.com/b", url);
}

static void test_explicit_http_port_is_kept(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("http://example.com:8080/a", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://example.com:8080/a", url);
}

static void test_query_is_preserved(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("http://example.com/a?q=1&x=2", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://example.com/a?q=1&x=2", url);
}

static void test_scheme_less_host_becomes_http(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("example.com/a", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://example.com/a", url);
}

static void test_ipv4_host_and_port(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("192.168.1.20:8080/status", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.20:8080/status", url);
}

static void test_absolute_ignores_base(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url("http://base.test/api/"));
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("http://example.com/a", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://example.com/a", url);
}

static void test_relative_without_base_is_unavailable(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_compose_url("/status", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_compose_url("status", url, sizeof(url)));
}

static void test_path_absolute_replaces_base_path(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url("http://host.test/api/v1"));
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("/health", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/health", url);
}

static void test_relative_path_replaces_last_segment(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url("http://host.test/api/v1"));
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("status", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/api/status", url);
}

static void test_directory_base_appends(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url("http://host.test/api/v1/"));
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("status", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/api/v1/status", url);
}

static void test_dot_segments_and_query_replacement(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url("http://host.test/api/v1"));
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("../x", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/x", url);
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("?q=2", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/api/v1?q=2", url);
}

static void test_base_rejects_query_and_keeps_previous(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url("http://host.test/api/"));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_set_base_url("http://host.test/api?q=1"));
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_get_base_url(url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/api/", url);
}

static void test_base_clear_and_relative_rejection(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url("example.com/api/"));
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_get_base_url(url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://example.com/api/", url);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_set_base_url("/relative"));
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url(""));
    TEST_ASSERT_EQUAL(ESP_OK, network_get_base_url(url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("", url);
}

static void test_rejected_urls(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url(NULL, url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("http://example.com/a#b", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("ftp://example.com", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("http://user:pass@example.com/", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("http://example.com:0/", url, sizeof(url)));
}

static void test_methods_headers_and_timeouts(void)
{
    TEST_ASSERT_TRUE(network_http_method_valid("GET"));
    TEST_ASSERT_TRUE(network_http_method_valid("DELETE"));
    TEST_ASSERT_FALSE(network_http_method_valid("get"));
    TEST_ASSERT_FALSE(network_http_method_valid(NULL));
    TEST_ASSERT_FALSE(network_http_method_allows_body("GET"));
    TEST_ASSERT_FALSE(network_http_method_allows_body("HEAD"));
    TEST_ASSERT_TRUE(network_http_method_allows_body("POST"));
    TEST_ASSERT_TRUE(network_http_header_valid("Authorization", "Bearer token"));
    TEST_ASSERT_TRUE(network_http_header_valid("X-Empty", ""));
    TEST_ASSERT_FALSE(network_http_header_valid("Bad Name", "x"));
    TEST_ASSERT_FALSE(network_http_header_valid("X-Line", "a\nb"));
    TEST_ASSERT_FALSE(network_http_header_valid("", "x"));
    TEST_ASSERT_TRUE(network_http_timeout_valid(100));
    TEST_ASSERT_TRUE(network_http_timeout_valid(60000));
    TEST_ASSERT_FALSE(network_http_timeout_valid(99));
    TEST_ASSERT_FALSE(network_http_timeout_valid(60001));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_absolute_http_adds_root);
    RUN_TEST(test_absolute_https_strips_default_port_and_lowercases_host);
    RUN_TEST(test_explicit_http_port_is_kept);
    RUN_TEST(test_query_is_preserved);
    RUN_TEST(test_scheme_less_host_becomes_http);
    RUN_TEST(test_ipv4_host_and_port);
    RUN_TEST(test_absolute_ignores_base);
    RUN_TEST(test_relative_without_base_is_unavailable);
    RUN_TEST(test_path_absolute_replaces_base_path);
    RUN_TEST(test_relative_path_replaces_last_segment);
    RUN_TEST(test_directory_base_appends);
    RUN_TEST(test_dot_segments_and_query_replacement);
    RUN_TEST(test_base_rejects_query_and_keeps_previous);
    RUN_TEST(test_base_clear_and_relative_rejection);
    RUN_TEST(test_rejected_urls);
    RUN_TEST(test_methods_headers_and_timeouts);
    return UNITY_END();
}
