#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @file service_diagnostics.h
     * @brief Shared diagnostics contract for service lifecycle/memory inspection.
     *
     * Canonical service IDs/names, state strings, metric labels/units, and n/a
     * semantics for consumers (`svc_mem`, `services`, `berry app`, etc.). Owned by
     * the service-memory-budget-observability plan (Milestone 1).
     */

    /** Canonical service identifier. */
    typedef enum
    {
        SERVICE_DIAG_ID_SYSTEM_HEAP = 0,
        SERVICE_DIAG_ID_BERRY_RUNTIME,
        SERVICE_DIAG_ID_NETWORK_HTTP,
        SERVICE_DIAG_ID_WIFI,
        SERVICE_DIAG_ID_BLUETOOTH,
        SERVICE_DIAG_ID_TEXT_CONSOLE,
        SERVICE_DIAG_ID_SHELL_INPUT,
        SERVICE_DIAG_ID_HTTP_SERVER,
        SERVICE_DIAG_ID_WEBSOCKET,
        SERVICE_DIAG_ID_NETWORK,
        SERVICE_DIAG_ID_TIME_SYNC,
        SERVICE_DIAG_ID_COUNT,
    } service_diag_id_t;

    /** Canonical lifecycle/state vocabulary for diagnostics reporting. */
    typedef enum
    {
        SERVICE_DIAG_STATE_NA = 0,
        SERVICE_DIAG_STATE_UNKNOWN,
        SERVICE_DIAG_STATE_UNINITIALIZED,
        SERVICE_DIAG_STATE_IDLE,
        SERVICE_DIAG_STATE_RUNNING,
        SERVICE_DIAG_STATE_READY,
        SERVICE_DIAG_STATE_CONNECTED,
        SERVICE_DIAG_STATE_STOPPED,
        SERVICE_DIAG_STATE_FAILED,
        SERVICE_DIAG_STATE_DISABLED_BY_PROFILE,
        SERVICE_DIAG_STATE_UNSUPPORTED_IN_PROFILE,
        SERVICE_DIAG_STATE_UNSUPPORTED_STOP,
        SERVICE_DIAG_STATE_UNSUPPORTED,
    } service_diag_state_t;

    /** Availability of a metric/value in a report. */
    typedef enum
    {
        SERVICE_DIAG_METRIC_NA = 0,      /**< Not applicable for this service/context. */
        SERVICE_DIAG_METRIC_UNKNOWN,     /**< Unknown (not currently observable). */
        SERVICE_DIAG_METRIC_UNSUPPORTED, /**< Unsupported by platform/service/capability. */
        SERVICE_DIAG_METRIC_OK,          /**< Value is valid. */
    } service_diag_metric_status_t;

    /** Size/value metric in bytes (or words where explicitly noted). */
    typedef struct
    {
        service_diag_metric_status_t status;
        uint32_t value;
        const char *reason; /**< Optional short reason for n/a/unknown/unsupported, may be NULL. */
    } service_diag_u32_metric_t;

    /** Boolean/state flag metric. */
    typedef struct
    {
        service_diag_metric_status_t status;
        bool value;
        const char *reason;
    } service_diag_bool_metric_t;

    /** System/default and internal heap snapshot (bytes). */
    typedef struct
    {
        service_diag_u32_metric_t default_free;
        service_diag_u32_metric_t default_min_free;
        service_diag_u32_metric_t default_largest_block;
        service_diag_u32_metric_t internal_free;
        service_diag_u32_metric_t internal_min_free;
        service_diag_u32_metric_t internal_largest_block;
    } service_diag_heap_snapshot_t;

    /** Berry runtime diagnostics snapshot. */
    typedef struct
    {
        service_diag_state_t state;
        service_diag_bool_metric_t initialized;
        service_diag_bool_metric_t vm_active;
        service_diag_u32_metric_t gc_managed_bytes;
        service_diag_u32_metric_t gc_step_rate;
        service_diag_heap_snapshot_t heap;
    } service_diag_berry_snapshot_t;

    /** Network/HTTP worker diagnostics snapshot. */
    typedef struct
    {
        service_diag_state_t state;
        service_diag_u32_metric_t worker_stack_cfg_bytes;
        service_diag_u32_metric_t worker_stack_hwm_bytes;
        service_diag_u32_metric_t worker_stack_unit_bytes;
        service_diag_bool_metric_t worker_hwm_available;
    } service_diag_network_snapshot_t;

    /** Wi-Fi diagnostics snapshot. */
    typedef struct
    {
        service_diag_state_t state;
    } service_diag_wifi_snapshot_t;

    /** Bluetooth diagnostics snapshot. */
    typedef struct
    {
        service_diag_state_t state;
    } service_diag_bluetooth_snapshot_t;

    /** Text console diagnostics snapshot. */
    typedef struct
    {
        service_diag_state_t state;
        service_diag_u32_metric_t cols;
        service_diag_u32_metric_t rows;
        service_diag_u32_metric_t font_size;
    } service_diag_text_console_snapshot_t;

    /** Shell input diagnostics snapshot. */
    typedef struct
    {
        service_diag_state_t state;
    } service_diag_shell_input_snapshot_t;

    /** HTTP server diagnostics snapshot. */
    typedef struct
    {
        service_diag_state_t state;
    } service_diag_http_server_snapshot_t;

    /** WebSocket diagnostics snapshot. */
    typedef struct
    {
        service_diag_state_t state;
        service_diag_u32_metric_t clients;
        service_diag_bool_metric_t http_server_available;
    } service_diag_websocket_snapshot_t;

    /** Network (coarse diagnostics/connectivity service) snapshot. */
    typedef struct
    {
        service_diag_state_t state;
        service_diag_bool_metric_t diagnostics_worker_sampled;
        service_diag_u32_metric_t diagnostics_worker_hwm_bytes;
    } service_diag_network_coarse_snapshot_t;

    /** Time sync diagnostics snapshot. */
    typedef struct
    {
        service_diag_state_t state;
        service_diag_bool_metric_t synced;
    } service_diag_time_sync_snapshot_t;

    /** Cross-service snapshot (on-demand, read-only). */
    typedef struct
    {
        service_diag_heap_snapshot_t system_heap;
        service_diag_berry_snapshot_t berry;
        service_diag_network_snapshot_t network_http;
        service_diag_wifi_snapshot_t wifi;
        service_diag_bluetooth_snapshot_t bluetooth;
        service_diag_text_console_snapshot_t text_console;
        service_diag_shell_input_snapshot_t shell_input;
        service_diag_http_server_snapshot_t http_server;
        service_diag_websocket_snapshot_t websocket;
        service_diag_network_coarse_snapshot_t network;
        service_diag_time_sync_snapshot_t time_sync;
    } service_diag_snapshot_t;

    /** @brief Return canonical service ID string. */
    const char *service_diag_id_str(service_diag_id_t id);

    /** @brief Return canonical service display name. */
    const char *service_diag_name(service_diag_id_t id);

    /** @brief Return canonical state string. */
    const char *service_diag_state_str(service_diag_state_t state);

    /** @brief Return canonical metric-status label for n/a/unknown/unsupported (or "ok"). */
    const char *service_diag_metric_status_str(service_diag_metric_status_t status);

    /**
     * @brief Collect an on-demand diagnostics snapshot for all in-scope services.
     *
     * Read-only and cheap; intended for manual `svc_mem` reporting.
     *
     * @param out Snapshot to populate (must not be NULL).
     * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out is NULL.
     */
    esp_err_t service_diag_collect(service_diag_snapshot_t *out);

    /**
     * @brief Print a grouped `svc_mem` report to stdout.
     *
     * Uses canonical IDs/names/states and metric units/labels. Existing `mem` and
     * `berry profile` behavior remain unchanged.
     *
     * @return ESP_OK on success.
     */
    esp_err_t service_diag_print_svc_mem(void);

#ifdef __cplusplus
}
#endif
