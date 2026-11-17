#include "service_diagnostics.h"

#include "berry_runtime.h"
#if CONFIG_BT_ENABLED
#include "bluetooth.h"
#endif
#include "http_server.h"
#include "network.h"
#include "system.h"
#include "text_console.h"
#include "time_sync.h"
#include "websocket.h"
#include "wifi.h"

#include <stdio.h>
#include <string.h>

static service_diag_u32_metric_t metric_u32_ok(uint32_t value)
{
    service_diag_u32_metric_t metric = {
        .status = SERVICE_DIAG_METRIC_OK,
        .value = value,
        .reason = NULL,
    };
    return metric;
}

static service_diag_u32_metric_t metric_u32_na(const char *reason)
{
    service_diag_u32_metric_t metric = {
        .status = SERVICE_DIAG_METRIC_NA,
        .value = 0,
        .reason = reason,
    };
    return metric;
}

static service_diag_u32_metric_t metric_u32_unknown(const char *reason)
{
    service_diag_u32_metric_t metric = {
        .status = SERVICE_DIAG_METRIC_UNKNOWN,
        .value = 0,
        .reason = reason,
    };
    return metric;
}

static service_diag_bool_metric_t metric_bool_ok(bool value)
{
    service_diag_bool_metric_t metric = {
        .status = SERVICE_DIAG_METRIC_OK,
        .value = value,
        .reason = NULL,
    };
    return metric;
}

static service_diag_bool_metric_t metric_bool_unknown(const char *reason)
{
    service_diag_bool_metric_t metric = {
        .status = SERVICE_DIAG_METRIC_UNKNOWN,
        .value = false,
        .reason = reason,
    };
    return metric;
}

const char *service_diag_id_str(service_diag_id_t id)
{
    switch (id)
    {
    case SERVICE_DIAG_ID_SYSTEM_HEAP:
        return "system_heap";
    case SERVICE_DIAG_ID_BERRY_RUNTIME:
        return "berry_runtime";
    case SERVICE_DIAG_ID_NETWORK_HTTP:
        return "network_http";
    case SERVICE_DIAG_ID_WIFI:
        return "wifi";
    case SERVICE_DIAG_ID_BLUETOOTH:
        return "bluetooth";
    case SERVICE_DIAG_ID_TEXT_CONSOLE:
        return "text_console";
    case SERVICE_DIAG_ID_SHELL_INPUT:
        return "shell_input";
    case SERVICE_DIAG_ID_HTTP_SERVER:
        return "http_server";
    case SERVICE_DIAG_ID_WEBSOCKET:
        return "websocket";
    case SERVICE_DIAG_ID_NETWORK:
        return "network";
    case SERVICE_DIAG_ID_TIME_SYNC:
        return "time_sync";
    default:
        return "unknown";
    }
}

const char *service_diag_name(service_diag_id_t id)
{
    switch (id)
    {
    case SERVICE_DIAG_ID_SYSTEM_HEAP:
        return "System heap";
    case SERVICE_DIAG_ID_BERRY_RUNTIME:
        return "Berry runtime";
    case SERVICE_DIAG_ID_NETWORK_HTTP:
        return "Network/HTTP worker";
    case SERVICE_DIAG_ID_WIFI:
        return "Wi-Fi";
    case SERVICE_DIAG_ID_BLUETOOTH:
        return "Bluetooth";
    case SERVICE_DIAG_ID_TEXT_CONSOLE:
        return "Text console";
    case SERVICE_DIAG_ID_SHELL_INPUT:
        return "Shell input";
    case SERVICE_DIAG_ID_HTTP_SERVER:
        return "HTTP server";
    case SERVICE_DIAG_ID_WEBSOCKET:
        return "WebSocket";
    case SERVICE_DIAG_ID_NETWORK:
        return "Network";
    case SERVICE_DIAG_ID_TIME_SYNC:
        return "Time sync";
    default:
        return "Unknown";
    }
}

const char *service_diag_state_str(service_diag_state_t state)
{
    switch (state)
    {
    case SERVICE_DIAG_STATE_UNINITIALIZED:
        return "uninitialized";
    case SERVICE_DIAG_STATE_IDLE:
        return "idle";
    case SERVICE_DIAG_STATE_RUNNING:
        return "running";
    case SERVICE_DIAG_STATE_READY:
        return "ready";
    case SERVICE_DIAG_STATE_CONNECTED:
        return "connected";
    case SERVICE_DIAG_STATE_STOPPED:
        return "stopped";
    case SERVICE_DIAG_STATE_FAILED:
        return "failed";
    case SERVICE_DIAG_STATE_DISABLED_BY_PROFILE:
        return "disabled-by-profile";
    case SERVICE_DIAG_STATE_UNSUPPORTED_IN_PROFILE:
        return "unsupported-in-profile";
    case SERVICE_DIAG_STATE_UNSUPPORTED_STOP:
        return "unsupported-stop";
    case SERVICE_DIAG_STATE_UNSUPPORTED:
        return "unsupported";
    case SERVICE_DIAG_STATE_UNKNOWN:
        return "unknown";
    case SERVICE_DIAG_STATE_NA:
    default:
        return "n/a";
    }
}

const char *service_diag_metric_status_str(service_diag_metric_status_t status)
{
    switch (status)
    {
    case SERVICE_DIAG_METRIC_OK:
        return "ok";
    case SERVICE_DIAG_METRIC_UNKNOWN:
        return "unknown";
    case SERVICE_DIAG_METRIC_UNSUPPORTED:
        return "unsupported";
    case SERVICE_DIAG_METRIC_NA:
    default:
        return "n/a";
    }
}

static void collect_system_heap(service_diag_heap_snapshot_t *heap)
{
    system_info_t info;
    esp_err_t err = system_get_info(&info);
    if (err != ESP_OK)
    {
        heap->default_free = metric_u32_unknown("system_get_info failed");
        heap->default_min_free = metric_u32_unknown("system_get_info failed");
        heap->default_largest_block = metric_u32_unknown("system_get_info failed");
        heap->internal_free = metric_u32_unknown("system_get_info failed");
        heap->internal_min_free = metric_u32_unknown("system_get_info failed");
        heap->internal_largest_block = metric_u32_unknown("system_get_info failed");
        return;
    }

    heap->default_free = metric_u32_ok((uint32_t)info.free_heap);
    heap->default_min_free = metric_u32_ok((uint32_t)info.min_free_heap);
    heap->default_largest_block = metric_u32_ok((uint32_t)info.max_alloc_block);
    heap->internal_free = metric_u32_ok((uint32_t)info.internal_free_heap);
    heap->internal_min_free = metric_u32_ok((uint32_t)info.internal_min_free_heap);
    heap->internal_largest_block = metric_u32_ok((uint32_t)info.internal_max_alloc_block);
}

static void collect_berry(service_diag_snapshot_t *out)
{
    berry_runtime_memory_info_t info;
    memset(&info, 0, sizeof(info));
    esp_err_t err = berry_runtime_get_memory_info(&info);
    if (err != ESP_OK)
    {
        out->berry.state = SERVICE_DIAG_STATE_UNKNOWN;
        out->berry.initialized = metric_bool_unknown("unavailable");
        out->berry.vm_active = metric_bool_unknown("unavailable");
        out->berry.gc_managed_bytes = metric_u32_unknown("unavailable");
        out->berry.gc_step_rate = metric_u32_unknown("unavailable");
        out->berry.heap.default_free = metric_u32_unknown("unavailable");
        out->berry.heap.default_min_free = metric_u32_unknown("unavailable");
        out->berry.heap.default_largest_block = metric_u32_unknown("unavailable");
        out->berry.heap.internal_free = metric_u32_unknown("unavailable");
        out->berry.heap.internal_min_free = metric_u32_unknown("unavailable");
        out->berry.heap.internal_largest_block = metric_u32_unknown("unavailable");
        return;
    }

    out->berry.initialized = metric_bool_ok(info.initialized);
    out->berry.vm_active = metric_bool_ok(info.initialized);
    out->berry.state = info.initialized ? SERVICE_DIAG_STATE_RUNNING : SERVICE_DIAG_STATE_UNINITIALIZED;
    out->berry.gc_managed_bytes = metric_u32_ok((uint32_t)info.gc_managed_bytes);
    out->berry.gc_step_rate = metric_u32_ok((uint32_t)info.gc_step_rate);
    out->berry.heap.default_free = metric_u32_ok((uint32_t)info.heap_free);
    out->berry.heap.default_min_free = metric_u32_ok((uint32_t)info.heap_min_free);
    out->berry.heap.default_largest_block = metric_u32_ok((uint32_t)info.heap_largest_block);
    out->berry.heap.internal_free = metric_u32_ok((uint32_t)info.internal_free);
    out->berry.heap.internal_min_free = metric_u32_ok((uint32_t)info.internal_min_free);
    out->berry.heap.internal_largest_block = metric_u32_ok((uint32_t)info.internal_largest_block);
}

static void collect_network_http(service_diag_network_snapshot_t *network)
{
    network_http_worker_stack_info_t info;
    memset(&info, 0, sizeof(info));
    esp_err_t err = network_http_get_worker_stack_info(&info);
    if (err != ESP_OK)
    {
        network->state = SERVICE_DIAG_STATE_UNKNOWN;
        network->worker_stack_cfg_bytes = metric_u32_unknown("unavailable");
        network->worker_stack_hwm_bytes = metric_u32_unknown("unavailable");
        network->worker_stack_unit_bytes = metric_u32_unknown("unavailable");
        network->worker_hwm_available = metric_bool_unknown("unavailable");
        return;
    }

    network->worker_stack_cfg_bytes = metric_u32_ok(info.configured_stack);
    network->worker_stack_unit_bytes = metric_u32_ok(info.stack_unit_bytes);
    network->worker_hwm_available = metric_bool_ok(info.available);
    if (info.available)
    {
        network->state = SERVICE_DIAG_STATE_READY;
        network->worker_stack_hwm_bytes = metric_u32_ok(info.hwm_bytes);
    }
    else
    {
        network->state = SERVICE_DIAG_STATE_IDLE;
        network->worker_stack_hwm_bytes = metric_u32_na("no sample yet");
    }
}

static void collect_wifi(service_diag_wifi_snapshot_t *wifi)
{
    const bool connected = wifi_is_connected();
    const bool started = wifi_is_service_started();
    const bool forced_off = wifi_is_forced_off_by_profile();

    if (connected)
    {
        wifi->state = SERVICE_DIAG_STATE_CONNECTED;
    }
    else if (started)
    {
        wifi->state = SERVICE_DIAG_STATE_RUNNING;
    }
    else if (forced_off)
    {
        wifi->state = SERVICE_DIAG_STATE_DISABLED_BY_PROFILE;
    }
    else
    {
        wifi->state = SERVICE_DIAG_STATE_STOPPED;
    }
}

static void collect_http_server(service_diag_http_server_snapshot_t *http_server)
{
    http_server->state = (http_server_get_handle() != NULL) ? SERVICE_DIAG_STATE_RUNNING : SERVICE_DIAG_STATE_STOPPED;
}

static void collect_websocket(service_diag_websocket_snapshot_t *websocket)
{
    const bool http_running = http_server_get_handle() != NULL;
    websocket->clients = metric_u32_ok((uint32_t)websocket_client_count());
    websocket->http_server_available = metric_bool_ok(http_running);
    websocket->state = http_running ? SERVICE_DIAG_STATE_RUNNING : SERVICE_DIAG_STATE_UNSUPPORTED;
}

static void collect_network(service_diag_network_coarse_snapshot_t *network)
{
    network_http_worker_stack_info_t info;
    memset(&info, 0, sizeof(info));
    const esp_err_t err = network_http_get_worker_stack_info(&info);
    if (err != ESP_OK)
    {
        network->state = SERVICE_DIAG_STATE_UNKNOWN;
        network->diagnostics_worker_sampled = metric_bool_unknown("unavailable");
        network->diagnostics_worker_hwm_bytes = metric_u32_unknown("unavailable");
        return;
    }

    network->diagnostics_worker_sampled = metric_bool_ok(info.available);
    network->diagnostics_worker_hwm_bytes =
        info.available ? metric_u32_ok(info.hwm_bytes) : metric_u32_na("no sample yet");
    network->state = info.available ? SERVICE_DIAG_STATE_READY : SERVICE_DIAG_STATE_IDLE;
}

static void collect_time_sync(service_diag_time_sync_snapshot_t *time_sync)
{
    const bool synced = time_sync_is_synced();
    time_sync->synced = metric_bool_ok(synced);
    time_sync->state = synced ? SERVICE_DIAG_STATE_READY : SERVICE_DIAG_STATE_RUNNING;
}

static void collect_bluetooth(service_diag_bluetooth_snapshot_t *bt)
{
#if CONFIG_BT_ENABLED
    if (bluetooth_is_connected())
    {
        bt->state = SERVICE_DIAG_STATE_CONNECTED;
        return;
    }
    if (bluetooth_is_enabled() || bluetooth_is_hid_ready())
    {
        bt->state = SERVICE_DIAG_STATE_RUNNING;
        return;
    }
    if (bluetooth_is_enabling())
    {
        bt->state = SERVICE_DIAG_STATE_UNKNOWN;
        return;
    }
    /* Enabled/disabled-by-profile distinction is not exposed by a dedicated API;
       report unknown when not active to avoid asserting a false stopped state. */
    bt->state = SERVICE_DIAG_STATE_UNKNOWN;
#else
    bt->state = SERVICE_DIAG_STATE_UNSUPPORTED_IN_PROFILE;
#endif
}

static void collect_text_console(service_diag_text_console_snapshot_t *console)
{
    /* Console size/font getters may be unavailable in some contexts and
       configured/buffer/HWM metrics are not cheaply exposed; report conservatively. */
    console->state = SERVICE_DIAG_STATE_UNKNOWN;
    console->cols = metric_u32_unknown("not exposed");
    console->rows = metric_u32_unknown("not exposed");
    console->font_size = metric_u32_unknown("not exposed");
}

static void collect_shell_input(service_diag_shell_input_snapshot_t *shell_input)
{
    /* Shell input is always-on once initialized in this firmware; detailed stack/HWM,
       transient read-state, and profile-disabled states are not exposed here without
       coupling diagnostics to shell internals. Report ready as best-effort baseline. */
    shell_input->state = SERVICE_DIAG_STATE_READY;
}

esp_err_t service_diag_collect(service_diag_snapshot_t *out)
{
    if (out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));
    collect_system_heap(&out->system_heap);
    collect_berry(out);
    collect_network_http(&out->network_http);
    collect_wifi(&out->wifi);
    collect_bluetooth(&out->bluetooth);
    collect_text_console(&out->text_console);
    collect_shell_input(&out->shell_input);
    collect_http_server(&out->http_server);
    collect_websocket(&out->websocket);
    collect_network(&out->network);
    collect_time_sync(&out->time_sync);
    return ESP_OK;
}

static void print_u32_metric_inline(const char *label, const service_diag_u32_metric_t *metric, const char *unit)
{
    if (metric->status == SERVICE_DIAG_METRIC_OK)
    {
        printf("  %s: %lu%s\n", label, (unsigned long)metric->value, unit != NULL ? unit : "");
        return;
    }
    if (metric->reason != NULL)
    {
        printf("  %s: %s (%s)\n", label, service_diag_metric_status_str(metric->status), metric->reason);
    }
    else
    {
        printf("  %s: %s\n", label, service_diag_metric_status_str(metric->status));
    }
}

static void print_bool_metric_inline(const char *label, const service_diag_bool_metric_t *metric)
{
    if (metric->status == SERVICE_DIAG_METRIC_OK)
    {
        printf("  %s: %s\n", label, metric->value ? "true" : "false");
        return;
    }
    if (metric->reason != NULL)
    {
        printf("  %s: %s (%s)\n", label, service_diag_metric_status_str(metric->status), metric->reason);
    }
    else
    {
        printf("  %s: %s\n", label, service_diag_metric_status_str(metric->status));
    }
}

esp_err_t service_diag_print_svc_mem(void)
{
    service_diag_snapshot_t snap;
    esp_err_t err = service_diag_collect(&snap);
    if (err != ESP_OK)
    {
        printf("svc_mem: collect failed (%s)\n", esp_err_to_name(err));
        return err;
    }

    printf("svc_mem (service memory/budget snapshot, on-demand)\n");
    printf("Units: bytes unless noted. n/a/unknown/unsupported values are reported explicitly.\n\n");

    printf("[%s] %s\n", service_diag_id_str(SERVICE_DIAG_ID_SYSTEM_HEAP),
           service_diag_name(SERVICE_DIAG_ID_SYSTEM_HEAP));
    print_u32_metric_inline("default_free", &snap.system_heap.default_free, " bytes");
    print_u32_metric_inline("default_min_free", &snap.system_heap.default_min_free, " bytes");
    print_u32_metric_inline("default_largest_block", &snap.system_heap.default_largest_block, " bytes");
    print_u32_metric_inline("internal_free", &snap.system_heap.internal_free, " bytes");
    print_u32_metric_inline("internal_min_free", &snap.system_heap.internal_min_free, " bytes");
    print_u32_metric_inline("internal_largest_block", &snap.system_heap.internal_largest_block, " bytes");
    printf("\n");

    printf("[%s] %s (state=%s)\n", service_diag_id_str(SERVICE_DIAG_ID_BERRY_RUNTIME),
           service_diag_name(SERVICE_DIAG_ID_BERRY_RUNTIME), service_diag_state_str(snap.berry.state));
    print_bool_metric_inline("initialized", &snap.berry.initialized);
    print_u32_metric_inline("gc_managed", &snap.berry.gc_managed_bytes, " bytes");
    print_u32_metric_inline("gc_step_rate", &snap.berry.gc_step_rate, "");
    print_u32_metric_inline("heap_free", &snap.berry.heap.default_free, " bytes");
    print_u32_metric_inline("heap_min_free", &snap.berry.heap.default_min_free, " bytes");
    print_u32_metric_inline("heap_largest_block", &snap.berry.heap.default_largest_block, " bytes");
    print_u32_metric_inline("internal_free", &snap.berry.heap.internal_free, " bytes");
    print_u32_metric_inline("internal_min_free", &snap.berry.heap.internal_min_free, " bytes");
    print_u32_metric_inline("internal_largest_block", &snap.berry.heap.internal_largest_block, " bytes");
    printf("\n");

    printf("[%s] %s (state=%s)\n", service_diag_id_str(SERVICE_DIAG_ID_NETWORK_HTTP),
           service_diag_name(SERVICE_DIAG_ID_NETWORK_HTTP), service_diag_state_str(snap.network_http.state));
    print_u32_metric_inline("worker_stack_cfg", &snap.network_http.worker_stack_cfg_bytes, " bytes");
    print_u32_metric_inline("worker_stack_hwm", &snap.network_http.worker_stack_hwm_bytes, " bytes");
    print_u32_metric_inline("worker_stack_unit", &snap.network_http.worker_stack_unit_bytes, " bytes/unit");
    print_bool_metric_inline("worker_hwm_available", &snap.network_http.worker_hwm_available);
    printf("  notes: bounded body/buffer limits per network.h (e.g. NETWORK_HTTP_BODY_MAX=%d)\n",
           NETWORK_HTTP_BODY_MAX);
    printf("\n");

    printf("[%s] %s (state=%s)\n", service_diag_id_str(SERVICE_DIAG_ID_WIFI), service_diag_name(SERVICE_DIAG_ID_WIFI),
           service_diag_state_str(snap.wifi.state));
    printf("  metrics: n/a (no cheap per-subsystem heap counters exposed; ESP-IDF reclaimability limited)\n\n");

    printf("[%s] %s (state=%s)\n", service_diag_id_str(SERVICE_DIAG_ID_BLUETOOTH),
           service_diag_name(SERVICE_DIAG_ID_BLUETOOTH), service_diag_state_str(snap.bluetooth.state));
    printf("  metrics: n/a (no cheap scan/buffer/task counters exposed)\n\n");

    printf("[%s] %s (state=%s)\n", service_diag_id_str(SERVICE_DIAG_ID_TEXT_CONSOLE),
           service_diag_name(SERVICE_DIAG_ID_TEXT_CONSOLE), service_diag_state_str(snap.text_console.state));
    print_u32_metric_inline("cols", &snap.text_console.cols, "");
    print_u32_metric_inline("rows", &snap.text_console.rows, "");
    print_u32_metric_inline("font_size", &snap.text_console.font_size, "");
    printf("  notes: task stack/HWM and buffer capacities not exposed as cheap accessors; semantics follow "
           "text_console lifecycle plan\n\n");

    printf("[%s] %s (state=%s)\n", service_diag_id_str(SERVICE_DIAG_ID_SHELL_INPUT),
           service_diag_name(SERVICE_DIAG_ID_SHELL_INPUT), service_diag_state_str(snap.shell_input.state));
    printf("  metrics: n/a (stack/HWM and input buffer/queue sizes not exposed as cheap accessors)\n\n");

    printf("Limitations: on-demand snapshot only; point-in-time values; ESP-IDF resources may be non-reclaimable.\n");
    return ESP_OK;
}
