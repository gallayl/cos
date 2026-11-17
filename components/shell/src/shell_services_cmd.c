#include "shell.h"

#include "bluetooth.h"
#include "service_diagnostics.h"
#include "wifi.h"

#include "esp_console.h"

#include <stdio.h>

static const char *bool_yes_no(bool value)
{
    return value ? "yes" : "no";
}

static const char *metric_yes_no_na(const service_diag_bool_metric_t *metric)
{
    if (metric == NULL || metric->status == SERVICE_DIAG_METRIC_NA)
    {
        return "n/a";
    }
    if (metric->status == SERVICE_DIAG_METRIC_UNKNOWN)
    {
        return "unknown";
    }
    if (metric->status == SERVICE_DIAG_METRIC_UNSUPPORTED)
    {
        return "unsupported";
    }
    return metric->value ? "yes" : "no";
}

static void print_clients_metric(const service_diag_u32_metric_t *metric)
{
    if (metric == NULL || metric->status == SERVICE_DIAG_METRIC_NA)
    {
        printf("n/a");
        return;
    }
    if (metric->status == SERVICE_DIAG_METRIC_UNKNOWN)
    {
        printf("unknown");
        return;
    }
    if (metric->status == SERVICE_DIAG_METRIC_UNSUPPORTED)
    {
        printf("unsupported");
        return;
    }
    printf("%lu", (unsigned long)metric->value);
}

static int cmd_services(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    service_diag_snapshot_t snap;
    const bool have_snap = (service_diag_collect(&snap) == ESP_OK);

    /* HTTP server */
    {
        const service_diag_state_t state = have_snap ? snap.http_server.state : SERVICE_DIAG_STATE_UNKNOWN;
        printf("[%s] %s: state=%s\n", service_diag_id_str(SERVICE_DIAG_ID_HTTP_SERVER),
               service_diag_name(SERVICE_DIAG_ID_HTTP_SERVER), service_diag_state_str(state));
    }

    /* WebSocket */
    {
        const service_diag_websocket_snapshot_t *ws = have_snap ? &snap.websocket : NULL;
        const service_diag_state_t state = have_snap ? snap.websocket.state : SERVICE_DIAG_STATE_UNKNOWN;
        printf("[%s] %s: state=%s clients=", service_diag_id_str(SERVICE_DIAG_ID_WEBSOCKET),
               service_diag_name(SERVICE_DIAG_ID_WEBSOCKET), service_diag_state_str(state));
        print_clients_metric(ws != NULL ? &ws->clients : NULL);
        printf(" http_server=%s\n", metric_yes_no_na(ws != NULL ? &ws->http_server_available : NULL));
    }

    /* Wi-Fi */
    {
        const service_diag_state_t state = have_snap ? snap.wifi.state : SERVICE_DIAG_STATE_UNKNOWN;
        printf("[%s] %s: state=%s connected=%s stored_credentials=%s\n", service_diag_id_str(SERVICE_DIAG_ID_WIFI),
               service_diag_name(SERVICE_DIAG_ID_WIFI), service_diag_state_str(state), bool_yes_no(wifi_is_connected()),
               bool_yes_no(wifi_has_stored_credentials()));
    }

    /* Bluetooth */
    {
        const service_diag_state_t state = have_snap ? snap.bluetooth.state : SERVICE_DIAG_STATE_UNKNOWN;
        const char *name = bluetooth_connected_device_name();
        printf("[%s] %s: state=%s enabled=%s enabling=%s connected=%s", service_diag_id_str(SERVICE_DIAG_ID_BLUETOOTH),
               service_diag_name(SERVICE_DIAG_ID_BLUETOOTH), service_diag_state_str(state),
               bool_yes_no(bluetooth_is_enabled()), bool_yes_no(bluetooth_is_enabling()),
               bool_yes_no(bluetooth_is_connected()));
        if (name != NULL && name[0] != '\0')
        {
            printf(" device=%s", name);
        }
        printf("\n");
    }

    /* Network (coarse diagnostics/worker state) */
    {
        const service_diag_network_coarse_snapshot_t *net = have_snap ? &snap.network : NULL;
        const service_diag_state_t state = have_snap ? snap.network.state : SERVICE_DIAG_STATE_UNKNOWN;
        printf("[%s] %s: state=%s diagnostics_worker=%s", service_diag_id_str(SERVICE_DIAG_ID_NETWORK),
               service_diag_name(SERVICE_DIAG_ID_NETWORK), service_diag_state_str(state),
               metric_yes_no_na(net != NULL ? &net->diagnostics_worker_sampled : NULL));
        if (net != NULL && net->diagnostics_worker_hwm_bytes.status == SERVICE_DIAG_METRIC_OK)
        {
            printf(" hwm_bytes=%lu", (unsigned long)net->diagnostics_worker_hwm_bytes.value);
        }
        printf("\n");
    }

    /* Time sync */
    {
        const service_diag_time_sync_snapshot_t *ts = have_snap ? &snap.time_sync : NULL;
        const service_diag_state_t state = have_snap ? snap.time_sync.state : SERVICE_DIAG_STATE_UNKNOWN;
        printf("[%s] %s: state=%s synced=%s\n", service_diag_id_str(SERVICE_DIAG_ID_TIME_SYNC),
               service_diag_name(SERVICE_DIAG_ID_TIME_SYNC), service_diag_state_str(state),
               metric_yes_no_na(ts != NULL ? &ts->synced : NULL));
    }

    return 0;
}

void shell_register_services_commands(void)
{
    const esp_console_cmd_t command = {
        .command = "services",
        .help = "Show one-shot status for core services",
        .hint = NULL,
        .func = &cmd_services,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&command));
}
