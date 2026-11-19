#include "http_server.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_api_route_prefix(void)
{
    TEST_ASSERT_TRUE(http_static_is_api_route("/api/"));
    TEST_ASSERT_TRUE(http_static_is_api_route("/api/files"));
    TEST_ASSERT_TRUE(http_static_is_api_route("/api/v1/status"));
}

static void test_ws_route_prefix_legacy_behavior(void)
{
    TEST_ASSERT_TRUE(http_static_is_api_route("/ws"));
    TEST_ASSERT_TRUE(http_static_is_api_route("/ws/"));
    TEST_ASSERT_TRUE(http_static_is_api_route("/ws/socket"));
    /* Preserves legacy prefix behavior (strncmp("/ws", 3)). */
    TEST_ASSERT_TRUE(http_static_is_api_route("/wsext"));
}

static void test_non_api_routes(void)
{
    TEST_ASSERT_FALSE(http_static_is_api_route("/"));
    TEST_ASSERT_FALSE(http_static_is_api_route("/index.html"));
    TEST_ASSERT_FALSE(http_static_is_api_route("/app.js"));
    TEST_ASSERT_FALSE(http_static_is_api_route("/appliance")); /* not /api/ */
    TEST_ASSERT_FALSE(http_static_is_api_route("/w"));         /* too short */
    TEST_ASSERT_FALSE(http_static_is_api_route("/w/other"));
}

static void test_null_uri(void)
{
    TEST_ASSERT_FALSE(http_static_is_api_route(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_api_route_prefix);
    RUN_TEST(test_ws_route_prefix_legacy_behavior);
    RUN_TEST(test_non_api_routes);
    RUN_TEST(test_null_uri);
    return UNITY_END();
}
