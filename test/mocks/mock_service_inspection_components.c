#include "esp_err.h"
#include "esp_http_server.h"

#include <stdbool.h>
#include <stddef.h>

static bool s_http_server_running = false;
static bool s_websocket_running = false;

void mock_set_http_server_running(bool running);
void mock_set_websocket_running(bool running);

void mock_set_http_server_running(bool running)
{
    s_http_server_running = running;
}

void mock_set_websocket_running(bool running)
{
    s_websocket_running = running;
}

#ifdef __cplusplus
extern "C"
{
#endif
    bool wifi_is_connected(void);
    bool wifi_has_stored_credentials(void);
    bool wifi_is_service_started(void);
    bool wifi_is_forced_off_by_profile(void);
    httpd_handle_t http_server_get_handle(void);
    bool http_server_is_running(void);
    size_t websocket_client_count(void);
    bool websocket_is_running(void);
    bool time_sync_is_synced(void);
#ifdef __cplusplus
}
#endif

httpd_handle_t http_server_get_handle(void)
{
    return s_http_server_running ? (httpd_handle_t)1 : NULL;
}

bool http_server_is_running(void)
{
    return s_http_server_running;
}

size_t websocket_client_count(void)
{
    return 0;
}

bool websocket_is_running(void)
{
    return s_websocket_running;
}

bool time_sync_is_synced(void)
{
    return false;
}

bool wifi_is_connected(void)
{
    return false;
}

bool wifi_has_stored_credentials(void)
{
    return false;
}

bool wifi_is_service_started(void)
{
    return false;
}

bool wifi_is_forced_off_by_profile(void)
{
    return false;
}