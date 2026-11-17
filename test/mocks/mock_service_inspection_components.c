#include "esp_err.h"
#include "esp_http_server.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif
    bool wifi_is_connected(void);
    bool wifi_has_stored_credentials(void);
    bool wifi_is_service_started(void);
    bool wifi_is_forced_off_by_profile(void);
    httpd_handle_t http_server_get_handle(void);
    size_t websocket_client_count(void);
    bool time_sync_is_synced(void);
#ifdef __cplusplus
}
#endif

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