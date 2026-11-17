#include "service_diagnostics.h"
#include "unity.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_canonical_service_ids_and_names(void)
{
    TEST_ASSERT_EQUAL_STRING("system_heap", service_diag_id_str(SERVICE_DIAG_ID_SYSTEM_HEAP));
    TEST_ASSERT_EQUAL_STRING("berry_runtime", service_diag_id_str(SERVICE_DIAG_ID_BERRY_RUNTIME));
    TEST_ASSERT_EQUAL_STRING("network_http", service_diag_id_str(SERVICE_DIAG_ID_NETWORK_HTTP));
    TEST_ASSERT_EQUAL_STRING("wifi", service_diag_id_str(SERVICE_DIAG_ID_WIFI));
    TEST_ASSERT_EQUAL_STRING("bluetooth", service_diag_id_str(SERVICE_DIAG_ID_BLUETOOTH));
    TEST_ASSERT_EQUAL_STRING("text_console", service_diag_id_str(SERVICE_DIAG_ID_TEXT_CONSOLE));
    TEST_ASSERT_EQUAL_STRING("shell_input", service_diag_id_str(SERVICE_DIAG_ID_SHELL_INPUT));
    TEST_ASSERT_EQUAL_STRING("System heap", service_diag_name(SERVICE_DIAG_ID_SYSTEM_HEAP));
    TEST_ASSERT_EQUAL_STRING("Shell input", service_diag_name(SERVICE_DIAG_ID_SHELL_INPUT));
}

static void test_canonical_states_and_metric_statuses(void)
{
    TEST_ASSERT_EQUAL_STRING("uninitialized", service_diag_state_str(SERVICE_DIAG_STATE_UNINITIALIZED));
    TEST_ASSERT_EQUAL_STRING("idle", service_diag_state_str(SERVICE_DIAG_STATE_IDLE));
    TEST_ASSERT_EQUAL_STRING("running", service_diag_state_str(SERVICE_DIAG_STATE_RUNNING));
    TEST_ASSERT_EQUAL_STRING("ready", service_diag_state_str(SERVICE_DIAG_STATE_READY));
    TEST_ASSERT_EQUAL_STRING("connected", service_diag_state_str(SERVICE_DIAG_STATE_CONNECTED));
    TEST_ASSERT_EQUAL_STRING("stopped", service_diag_state_str(SERVICE_DIAG_STATE_STOPPED));
    TEST_ASSERT_EQUAL_STRING("failed", service_diag_state_str(SERVICE_DIAG_STATE_FAILED));
    TEST_ASSERT_EQUAL_STRING("disabled-by-profile", service_diag_state_str(SERVICE_DIAG_STATE_DISABLED_BY_PROFILE));
    TEST_ASSERT_EQUAL_STRING("unsupported-in-profile", service_diag_state_str(SERVICE_DIAG_STATE_UNSUPPORTED_IN_PROFILE));
    TEST_ASSERT_EQUAL_STRING("unsupported-stop", service_diag_state_str(SERVICE_DIAG_STATE_UNSUPPORTED_STOP));
    TEST_ASSERT_EQUAL_STRING("unsupported", service_diag_state_str(SERVICE_DIAG_STATE_UNSUPPORTED));
    TEST_ASSERT_EQUAL_STRING("unknown", service_diag_state_str(SERVICE_DIAG_STATE_UNKNOWN));
    TEST_ASSERT_EQUAL_STRING("n/a", service_diag_state_str(SERVICE_DIAG_STATE_NA));

    TEST_ASSERT_EQUAL_STRING("ok", service_diag_metric_status_str(SERVICE_DIAG_METRIC_OK));
    TEST_ASSERT_EQUAL_STRING("unknown", service_diag_metric_status_str(SERVICE_DIAG_METRIC_UNKNOWN));
    TEST_ASSERT_EQUAL_STRING("unsupported", service_diag_metric_status_str(SERVICE_DIAG_METRIC_UNSUPPORTED));
    TEST_ASSERT_EQUAL_STRING("n/a", service_diag_metric_status_str(SERVICE_DIAG_METRIC_NA));
}

static void test_collect_rejects_null(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, service_diag_collect(NULL));
}

static void test_collect_populates_snapshot(void)
{
    service_diag_snapshot_t snap;
    memset(&snap, 0xA5, sizeof(snap));
    TEST_ASSERT_EQUAL(ESP_OK, service_diag_collect(&snap));

    /* System heap and Berry runtime collectors return ok/known metrics with host mocks/system APIs. */
    TEST_ASSERT_TRUE(snap.system_heap.default_free.status == SERVICE_DIAG_METRIC_OK ||
                     snap.system_heap.default_free.status == SERVICE_DIAG_METRIC_UNKNOWN);
    TEST_ASSERT_TRUE(snap.berry.state == SERVICE_DIAG_STATE_RUNNING ||
                     snap.berry.state == SERVICE_DIAG_STATE_UNINITIALIZED ||
                     snap.berry.state == SERVICE_DIAG_STATE_UNKNOWN);

    /* Best-effort conservative collectors report canonical unknown/n-a where accessors are unavailable. */
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_UNKNOWN, snap.wifi.state);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_UNKNOWN, snap.text_console.state);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_METRIC_UNKNOWN, snap.text_console.cols.status);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_READY, snap.shell_input.state);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_canonical_service_ids_and_names);
    RUN_TEST(test_canonical_states_and_metric_statuses);
    RUN_TEST(test_collect_rejects_null);
    RUN_TEST(test_collect_populates_snapshot);
    return UNITY_END();
}
