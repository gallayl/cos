#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

#define HTTP_AUTH_MAX_USER 32
#define HTTP_AUTH_MAX_PASS 64

esp_err_t http_auth_init(void);
esp_err_t http_auth_set_credentials(const char *username, const char *password);
const char *http_auth_get_username(void);
esp_err_t http_auth_check(httpd_req_t *req);
