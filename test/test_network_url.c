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
    TEST_ASSERT_TRUE(network_http_method_allows_body("PUT"));
    TEST_ASSERT_TRUE(network_http_method_allows_body("PATCH"));
    TEST_ASSERT_TRUE(network_http_method_allows_body("DELETE"));
    TEST_ASSERT_FALSE(network_http_method_allows_body("OPTIONS"));
    TEST_ASSERT_TRUE(network_http_header_valid("Authorization", "Bearer token"));
    TEST_ASSERT_TRUE(network_http_header_valid("X-Empty", ""));
    TEST_ASSERT_FALSE(network_http_header_valid("Bad Name", "x"));
    TEST_ASSERT_FALSE(network_http_header_valid("X-Line", "a\nb"));
    TEST_ASSERT_FALSE(network_http_header_valid("", "x"));
    TEST_ASSERT_FALSE(network_http_header_valid(NULL, "x"));
    TEST_ASSERT_FALSE(network_http_header_valid("X-Test", NULL));
    TEST_ASSERT_FALSE(network_http_header_valid("X-Ctrl", "a\rb"));
    TEST_ASSERT_FALSE(network_http_header_valid("X-Tab", "a\tb"));
    char long_name[NETWORK_HTTP_HEADER_NAME_MAX + 2];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    TEST_ASSERT_FALSE(network_http_header_valid(long_name, "x"));
    long_name[NETWORK_HTTP_HEADER_NAME_MAX] = '\0';
    TEST_ASSERT_TRUE(network_http_header_valid(long_name, "x"));
    char long_value[NETWORK_HTTP_HEADER_VALUE_MAX + 2];
    memset(long_value, 'b', sizeof(long_value) - 1);
    long_value[sizeof(long_value) - 1] = '\0';
    TEST_ASSERT_FALSE(network_http_header_valid("X-Test", long_value));
    long_value[NETWORK_HTTP_HEADER_VALUE_MAX] = '\0';
    TEST_ASSERT_TRUE(network_http_header_valid("X-Test", long_value));
    TEST_ASSERT_TRUE(network_http_timeout_valid(NETWORK_HTTP_TIMEOUT_MIN_MS));
    TEST_ASSERT_TRUE(network_http_timeout_valid(NETWORK_HTTP_TIMEOUT_MAX_MS));
    TEST_ASSERT_FALSE(network_http_timeout_valid(NETWORK_HTTP_TIMEOUT_MIN_MS - 1));
    TEST_ASSERT_FALSE(network_http_timeout_valid(NETWORK_HTTP_TIMEOUT_MAX_MS + 1));
}

static void test_absolute_localhost_and_ipv4_port_normalization(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("http://localhost/api", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://localhost/api", url);
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("http://LOCALHOST:80/api", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://localhost/api", url);
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("https://localhost:443/api", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("https://localhost/api", url);
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("http://192.168.1.1:80/", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.1/", url);
}

static void test_relative_resolution_edge_cases(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url("http://host.test/api/v1/index.html"));
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("?q=2", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/api/v1/index.html?q=2", url);
    /* Empty relative reference is rejected as invalid input by compose API. */
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("./x", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/api/v1/x", url);
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("../../../z", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/z", url);
    TEST_ASSERT_EQUAL(ESP_OK, network_compose_url("/a/./b/../c", url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/a/c", url);
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url(""));
}

static void test_base_and_get_base_url_contracts(void)
{
    char url[NETWORK_URL_MAX + 1];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_set_base_url(NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_get_base_url(NULL, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_get_base_url(url, 0));
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url("http://host.test/api/"));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, network_get_base_url(url, 5));
    TEST_ASSERT_EQUAL(ESP_OK, network_get_base_url(url, sizeof(url)));
    TEST_ASSERT_EQUAL_STRING("http://host.test/api/", url);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_set_base_url("http://host.test/api#frag"));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_set_base_url("http://user@example.com/api"));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_set_base_url("ftp://host.test/api"));
}

static void test_compose_rejects_invalid_edge_cases(void)
{
    char url[NETWORK_URL_MAX + 1];
    char small[8];
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("http://example.com", NULL, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("http://example.com", url, 0));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("http://exa mple.com", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("http://example.com:65536/", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("http://example.com:abc/", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("http://-bad.example.com/", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_compose_url("http://bad-.example.com/", url, sizeof(url)));
    /* Malformed scheme without "://" is treated as relative input; without base it is unavailable. */
    TEST_ASSERT_EQUAL(ESP_OK, network_set_base_url(""));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_compose_url("http:/missing-slashes", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_compose_url("localhost/path", url, sizeof(url)));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, network_compose_url("http://example.com/very/long/path", small, sizeof(small)));
    char too_long[NETWORK_URL_MAX + 2];
    memset(too_long, 'a', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = '\0';
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, network_compose_url(too_long, url, sizeof(url)));
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
    RUN_TEST(test_absolute_localhost_and_ipv4_port_normalization);
    RUN_TEST(test_relative_resolution_edge_cases);
    RUN_TEST(test_base_and_get_base_url_contracts);
    RUN_TEST(test_compose_rejects_invalid_edge_cases);
    return UNITY_END();
}
