#include "mock_httpd.h"
#include "esp_http_server.h"

#include <string.h>

#define MOCK_MAX_HEADERS 8
#define MOCK_HDR_FIELD_LEN 64
#define MOCK_HDR_VALUE_LEN 256

static struct
{
    char field[MOCK_HDR_FIELD_LEN];
    char value[MOCK_HDR_VALUE_LEN];
    bool set;
} s_headers[MOCK_MAX_HEADERS];

static int s_dummy_server = 1;
static char s_last_status[64];
static char s_last_type[64];

void mock_httpd_reset(void)
{
    memset(s_headers, 0, sizeof(s_headers));
    s_last_status[0] = '\0';
    s_last_type[0] = '\0';
}

void mock_httpd_set_header(const char *field, const char *value)
{
    for (int i = 0; i < MOCK_MAX_HEADERS; i++)
    {
        if (!s_headers[i].set)
        {
            strncpy(s_headers[i].field, field, MOCK_HDR_FIELD_LEN - 1);
            strncpy(s_headers[i].value, value, MOCK_HDR_VALUE_LEN - 1);
            s_headers[i].set = true;
            return;
        }
    }
}

esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config)
{
    (void)config;
    *handle = (httpd_handle_t)&s_dummy_server;
    return ESP_OK;
}

esp_err_t httpd_stop(httpd_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}

esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri)
{
    (void)handle;
    (void)uri;
    return ESP_OK;
}

esp_err_t httpd_register_err_handler(httpd_handle_t handle, httpd_err_code_t error,
                                     httpd_err_handler_func_t handler)
{
    (void)handle;
    (void)error;
    (void)handler;
    return ESP_OK;
}

esp_err_t httpd_resp_set_type(httpd_req_t *r, const char *type)
{
    (void)r;
    strncpy(s_last_type, type, sizeof(s_last_type) - 1);
    return ESP_OK;
}

esp_err_t httpd_resp_set_status(httpd_req_t *r, const char *status)
{
    (void)r;
    strncpy(s_last_status, status, sizeof(s_last_status) - 1);
    return ESP_OK;
}

esp_err_t httpd_resp_set_hdr(httpd_req_t *r, const char *field, const char *value)
{
    (void)r;
    (void)field;
    (void)value;
    return ESP_OK;
}

esp_err_t httpd_resp_send(httpd_req_t *r, const char *buf, int buf_len)
{
    (void)r;
    (void)buf;
    (void)buf_len;
    return ESP_OK;
}

esp_err_t httpd_resp_sendstr_chunk(httpd_req_t *r, const char *chunk)
{
    (void)r;
    (void)chunk;
    return ESP_OK;
}

esp_err_t httpd_resp_send_chunk(httpd_req_t *r, const char *chunk, size_t len)
{
    (void)r;
    (void)chunk;
    (void)len;
    return ESP_OK;
}

esp_err_t httpd_resp_send_err(httpd_req_t *r, httpd_err_code_t error, const char *msg)
{
    (void)r;
    (void)error;
    (void)msg;
    return ESP_OK;
}

esp_err_t httpd_req_get_url_query_str(httpd_req_t *r, char *buf, size_t buf_len)
{
    (void)r;
    (void)buf;
    (void)buf_len;
    return ESP_ERR_NOT_FOUND;
}

esp_err_t httpd_query_key_value(const char *qry, const char *key, char *val, size_t val_size)
{
    (void)qry;
    (void)key;
    (void)val;
    (void)val_size;
    return ESP_ERR_NOT_FOUND;
}

esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *r, const char *field, char *val, size_t val_size)
{
    (void)r;
    for (int i = 0; i < MOCK_MAX_HEADERS; i++)
    {
        if (s_headers[i].set && strcmp(s_headers[i].field, field) == 0)
        {
            strncpy(val, s_headers[i].value, val_size - 1);
            val[val_size - 1] = '\0';
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

int httpd_req_recv(httpd_req_t *r, char *buf, size_t buf_len)
{
    (void)r;
    (void)buf;
    (void)buf_len;
    return 0;
}

int httpd_req_to_sockfd(httpd_req_t *r)
{
    (void)r;
    return 42;
}

esp_err_t httpd_ws_recv_frame(httpd_req_t *req, httpd_ws_frame_t *frame, size_t max_len)
{
    (void)req;
    (void)frame;
    (void)max_len;
    return ESP_OK;
}

esp_err_t httpd_ws_send_frame(httpd_req_t *req, httpd_ws_frame_t *frame)
{
    (void)req;
    (void)frame;
    return ESP_OK;
}

esp_err_t httpd_ws_send_frame_async(httpd_handle_t hd, int fd, httpd_ws_frame_t *frame)
{
    (void)hd;
    (void)fd;
    (void)frame;
    return ESP_OK;
}

uint32_t esp_get_free_heap_size(void)
{
    return 200000;
}
