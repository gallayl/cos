#include "http_server.h"
#include "time_sync.h"
#include "websocket.h"
#include "wifi.h"

#include <stdbool.h>
#include <stddef.h>

httpd_handle_t http_server_get_handle(void)
{
    return NULL;
}

size_t websocket_client_count(void)
{
    return 0;
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