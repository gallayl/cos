#include "service_lifecycle.h"
#include "unity.h"

#include <stddef.h>

static int s_a_init_calls;
static int s_b_init_calls;
static int s_c_init_calls;
static int s_stop_calls;
static esp_err_t s_a_result;
static esp_err_t s_b_result;

static esp_err_t init_a(service_descriptor_t *svc)
{
    (void)svc;
    s_a_init_calls++;
    return s_a_result;
}

static esp_err_t init_b(service_descriptor_t *svc)
{
    (void)svc;
    s_b_init_calls++;
    return s_b_result;
}

static esp_err_t init_c(service_descriptor_t *svc)
{
    (void)svc;
    s_c_init_calls++;
    return ESP_OK;
}

static esp_err_t stop_ok(service_descriptor_t *svc)
{
    (void)svc;
    s_stop_calls++;
    return ESP_OK;
}

void setUp(void)
{
    s_a_init_calls = 0;
    s_b_init_calls = 0;
    s_c_init_calls = 0;
    s_stop_calls = 0;
    s_a_result = ESP_OK;
    s_b_result = ESP_OK;
}

void tearDown(void) {}

static void test_boot_runs_in_order_when_enabled(void)
{
    service_descriptor_t services[] = {
        {.id = "a", .name = "A", .init = init_a, .capabilities = SERVICE_CAP_CAN_START, .enabled = true},
        {.id = "b", .name = "B", .init = init_b, .capabilities = SERVICE_CAP_CAN_START, .enabled = true},
    };

    service_lifecycle_boot(services, 2);

    TEST_ASSERT_EQUAL_INT(1, s_a_init_calls);
    TEST_ASSERT_EQUAL_INT(1, s_b_init_calls);
    TEST_ASSERT_EQUAL(SERVICE_LIFECYCLE_STARTED, services[0].state);
    TEST_ASSERT_EQUAL(SERVICE_LIFECYCLE_STARTED, services[1].state);
}

static void test_boot_fails_when_dependency_not_started(void)
{
    static const char *const deps[] = {"a"};
    service_descriptor_t services[] = {
        {.id = "a", .name = "A", .init = init_a, .capabilities = SERVICE_CAP_CAN_START, .enabled = true},
        {.id = "b",
         .name = "B",
         .depends_on = deps,
         .depends_on_count = 1,
         .init = init_b,
         .capabilities = SERVICE_CAP_CAN_START,
         .enabled = true},
    };
    s_a_result = ESP_FAIL;

    service_lifecycle_boot(services, 2);

    TEST_ASSERT_EQUAL_INT(1, s_a_init_calls);
    TEST_ASSERT_EQUAL_INT(0, s_b_init_calls);
    TEST_ASSERT_EQUAL(SERVICE_LIFECYCLE_FAILED, services[0].state);
    TEST_ASSERT_EQUAL(SERVICE_LIFECYCLE_FAILED, services[1].state);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, services[1].last_result);
}

static void test_disabled_service_remains_uninitialized(void)
{
    service_descriptor_t services[] = {
        {.id = "a", .name = "A", .init = init_a, .capabilities = SERVICE_CAP_CAN_START, .enabled = false},
    };

    service_lifecycle_boot(services, 1);

    TEST_ASSERT_EQUAL_INT(0, s_a_init_calls);
    TEST_ASSERT_EQUAL(SERVICE_LIFECYCLE_UNINITIALIZED, services[0].state);
}

static void test_stop_all_best_effort_reverse_and_unsupported(void)
{
    static const char *const deps[] = {"a"};
    service_descriptor_t services[] = {
        {.id = "a",
         .name = "A",
         .init = init_a,
         .stop = stop_ok,
         .capabilities = SERVICE_CAP_CAN_START | SERVICE_CAP_CAN_STOP,
         .enabled = true},
        {.id = "b",
         .name = "B",
         .depends_on = deps,
         .depends_on_count = 1,
         .init = init_b,
         .capabilities = SERVICE_CAP_CAN_START,
         .enabled = true},
        {.id = "c", .name = "C", .init = init_c, .capabilities = SERVICE_CAP_CAN_START, .enabled = false},
    };

    service_lifecycle_boot(services, 3);
    TEST_ASSERT_EQUAL(SERVICE_LIFECYCLE_STARTED, services[0].state);
    TEST_ASSERT_EQUAL(SERVICE_LIFECYCLE_STARTED, services[1].state);

    service_lifecycle_stop_all(services, 3);

    TEST_ASSERT_EQUAL_INT(1, s_stop_calls);
    TEST_ASSERT_EQUAL(SERVICE_LIFECYCLE_STOPPED, services[0].state);
    TEST_ASSERT_EQUAL(SERVICE_LIFECYCLE_UNSUPPORTED_STOP, services[1].state);
    TEST_ASSERT_EQUAL(SERVICE_LIFECYCLE_UNINITIALIZED, services[2].state);
}

static void test_find_returns_matching_descriptor(void)
{
    service_descriptor_t services[] = {
        {.id = "a", .name = "A", .enabled = true},
        {.id = "b", .name = "B", .enabled = true},
    };

    service_descriptor_t *svc = service_lifecycle_find(services, 2, "b");
    TEST_ASSERT_NOT_NULL(svc);
    TEST_ASSERT_EQUAL_STRING("B", svc->name);
    TEST_ASSERT_NULL(service_lifecycle_find(services, 2, "missing"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_boot_runs_in_order_when_enabled);
    RUN_TEST(test_boot_fails_when_dependency_not_started);
    RUN_TEST(test_disabled_service_remains_uninitialized);
    RUN_TEST(test_stop_all_best_effort_reverse_and_unsupported);
    RUN_TEST(test_find_returns_matching_descriptor);
    return UNITY_END();
}
