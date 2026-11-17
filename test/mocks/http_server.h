#pragma once

#include "esp_http_server.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

httpd_handle_t http_server_get_handle(void);

#ifdef __cplusplus
}
#endif