#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void *httpd_handle_t;
typedef int http_method_t;

#define HTTP_GET  0
#define HTTP_POST 1

#define HTTPD_RESP_USE_STRLEN -1
#define HTTPD_SOCK_ERR_TIMEOUT -1
#define HTTPD_200 "200 OK"

typedef enum
{
    HTTPD_404_NOT_FOUND = 404,
    HTTPD_500_INTERNAL_SERVER_ERROR = 500,
} httpd_err_code_t;

typedef enum
{
    HTTPD_WS_TYPE_TEXT = 1,
    HTTPD_WS_TYPE_CLOSE = 8,
} httpd_ws_type_t;

typedef struct
{
    httpd_ws_type_t type;
    uint8_t *payload;
    size_t len;
} httpd_ws_frame_t;

typedef struct httpd_req
{
    const char *uri;
    int method;
    size_t content_len;
    void *user_ctx;
} httpd_req_t;

typedef esp_err_t (*httpd_uri_func_t)(httpd_req_t *r);
typedef esp_err_t (*httpd_err_handler_func_t)(httpd_req_t *r, httpd_err_code_t err);
typedef void (*httpd_close_func_t)(httpd_handle_t hd, int sockfd);

typedef struct
{
    const char *uri;
    http_method_t method;
    httpd_uri_func_t handler;
    void *user_ctx;
    bool is_websocket;
} httpd_uri_t;

typedef struct
{
    unsigned server_port;
    unsigned max_uri_handlers;
    bool lru_purge_enable;
    unsigned stack_size;
    httpd_close_func_t close_fn;
} httpd_config_t;

#define HTTPD_DEFAULT_CONFIG()       \
    {                                \
        .server_port = 80,           \
        .max_uri_handlers = 8,       \
        .lru_purge_enable = false,   \
        .stack_size = 4096,          \
        .close_fn = NULL,            \
    }

esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config);
esp_err_t httpd_stop(httpd_handle_t handle);
esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri);
esp_err_t httpd_register_err_handler(httpd_handle_t handle, httpd_err_code_t error, httpd_err_handler_func_t handler);
esp_err_t httpd_resp_set_type(httpd_req_t *r, const char *type);
esp_err_t httpd_resp_set_status(httpd_req_t *r, const char *status);
esp_err_t httpd_resp_set_hdr(httpd_req_t *r, const char *field, const char *value);
esp_err_t httpd_resp_send(httpd_req_t *r, const char *buf, int buf_len);
esp_err_t httpd_resp_sendstr_chunk(httpd_req_t *r, const char *chunk);
esp_err_t httpd_resp_send_chunk(httpd_req_t *r, const char *chunk, size_t len);
esp_err_t httpd_resp_send_err(httpd_req_t *r, httpd_err_code_t error, const char *msg);
esp_err_t httpd_req_get_url_query_str(httpd_req_t *r, char *buf, size_t buf_len);
esp_err_t httpd_query_key_value(const char *qry, const char *key, char *val, size_t val_size);
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *r, const char *field, char *val, size_t val_size);
int httpd_req_recv(httpd_req_t *r, char *buf, size_t buf_len);
int httpd_req_to_sockfd(httpd_req_t *r);
esp_err_t httpd_ws_recv_frame(httpd_req_t *req, httpd_ws_frame_t *frame, size_t max_len);
esp_err_t httpd_ws_send_frame(httpd_req_t *req, httpd_ws_frame_t *frame);
esp_err_t httpd_ws_send_frame_async(httpd_handle_t hd, int fd, httpd_ws_frame_t *frame);

uint32_t esp_get_free_heap_size(void);
