#include "service_diagnostics.h"
#include "unity.h"

#include <string.h>

void mock_set_http_server_running(bool running);
void mock_set_websocket_running(bool running);

void setUp(void)
{
    mock_set_http_server_running(false);
    mock_set_websocket_running(false);
}
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
    TEST_ASSERT_EQUAL_STRING("http_server", service_diag_id_str(SERVICE_DIAG_ID_HTTP_SERVER));
    TEST_ASSERT_EQUAL_STRING("websocket", service_diag_id_str(SERVICE_DIAG_ID_WEBSOCKET));
    TEST_ASSERT_EQUAL_STRING("network", service_diag_id_str(SERVICE_DIAG_ID_NETWORK));
    TEST_ASSERT_EQUAL_STRING("time_sync", service_diag_id_str(SERVICE_DIAG_ID_TIME_SYNC));
    TEST_ASSERT_EQUAL_STRING("System heap", service_diag_name(SERVICE_DIAG_ID_SYSTEM_HEAP));
    TEST_ASSERT_EQUAL_STRING("Shell input", service_diag_name(SERVICE_DIAG_ID_SHELL_INPUT));
    TEST_ASSERT_EQUAL_STRING("HTTP server", service_diag_name(SERVICE_DIAG_ID_HTTP_SERVER));
    TEST_ASSERT_EQUAL_STRING("WebSocket", service_diag_name(SERVICE_DIAG_ID_WEBSOCKET));
    TEST_ASSERT_EQUAL_STRING("Network", service_diag_name(SERVICE_DIAG_ID_NETWORK));
    TEST_ASSERT_EQUAL_STRING("Time sync", service_diag_name(SERVICE_DIAG_ID_TIME_SYNC));
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

    /* Best-effort collectors use cheap APIs/mocks: Wi-Fi stopped, HTTP server stopped, etc. */
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_STOPPED, snap.wifi.state);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_STOPPED, snap.http_server.state);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_UNSUPPORTED, snap.websocket.state);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_METRIC_OK, snap.websocket.clients.status);
    TEST_ASSERT_EQUAL_UINT32(0, snap.websocket.clients.value);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_METRIC_OK, snap.time_sync.synced.status);
    TEST_ASSERT_FALSE(snap.time_sync.synced.value);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_UNKNOWN, snap.text_console.state);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_METRIC_UNKNOWN, snap.text_console.cols.status);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_READY, snap.shell_input.state);
}

static void test_collect_reports_http_websocket_runtime_states(void)
{
    service_diag_snapshot_t snap;

    mock_set_http_server_running(true);
    mock_set_websocket_running(false);
    TEST_ASSERT_EQUAL(ESP_OK, service_diag_collect(&snap));
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_RUNNING, snap.http_server.state);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_STOPPED, snap.websocket.state);

    mock_set_websocket_running(true);
    TEST_ASSERT_EQUAL(ESP_OK, service_diag_collect(&snap));
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_RUNNING, snap.http_server.state);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_RUNNING, snap.websocket.state);

    mock_set_http_server_running(false);
    mock_set_websocket_running(false);
    TEST_ASSERT_EQUAL(ESP_OK, service_diag_collect(&snap));
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_STOPPED, snap.http_server.state);
    TEST_ASSERT_EQUAL(SERVICE_DIAG_STATE_UNSUPPORTED, snap.websocket.state);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_canonical_service_ids_and_names);
    RUN_TEST(test_canonical_states_and_metric_statuses);
    RUN_TEST(test_collect_rejects_null);
    RUN_TEST(test_collect_populates_snapshot);
    RUN_TEST(test_collect_reports_http_websocket_runtime_states);
    return UNITY_END();
}
