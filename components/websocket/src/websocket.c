#include "websocket.h"
#include "http_server.h"

#include "esp_console.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "websocket";

static int s_clients[WEBSOCKET_MAX_CLIENTS];
static size_t s_client_count = 0;
static SemaphoreHandle_t s_mutex = NULL;

static void add_client(int fd)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_client_count < WEBSOCKET_MAX_CLIENTS)
    {
        s_clients[s_client_count++] = fd;
    }
    xSemaphoreGive(s_mutex);
}

static void remove_client(int fd)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (size_t i = 0; i < s_client_count; i++)
    {
        if (s_clients[i] == fd)
        {
            s_clients[i] = s_clients[s_client_count - 1];
            s_client_count--;
            break;
        }
    }
    xSemaphoreGive(s_mutex);
}

void websocket_on_session_close(int sockfd)
{
    remove_client(sockfd);
}

size_t websocket_client_count(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    size_t count = s_client_count;
    xSemaphoreGive(s_mutex);
    return count;
}

esp_err_t websocket_broadcast(const char *msg, size_t len)
{
    httpd_handle_t server = http_server_get_handle();
    if (!server || !msg)
    {
        return ESP_ERR_INVALID_STATE;
    }

    httpd_ws_frame_t frame = {
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)msg,
        .len = len,
    };

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (size_t i = 0; i < s_client_count;)
    {
        if (httpd_ws_send_frame_async(server, s_clients[i], &frame) != ESP_OK)
        {
            ESP_LOGW(TAG, "Send failed for fd %d, removing", s_clients[i]);
            s_clients[i] = s_clients[s_client_count - 1];
            s_client_count--;
        }
        else
        {
            i++;
        }
    }
    xSemaphoreGive(s_mutex);

    return ESP_OK;
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        if (http_auth_check(req) != ESP_OK)
        {
            return ESP_OK;
        }
        int fd = httpd_req_to_sockfd(req);
        add_client(fd);
        ESP_LOGI(TAG, "Client connected (fd=%d, total=%u)", fd, (unsigned)websocket_client_count());
        return ESP_OK;
    }

    /* Broadcast-only: ignore incoming frames */
    httpd_ws_frame_t frame = {0};
    frame.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (frame.type == HTTPD_WS_TYPE_CLOSE)
    {
        int fd = httpd_req_to_sockfd(req);
        remove_client(fd);
        ESP_LOGI(TAG, "Client closed (fd=%d)", fd);
    }

    /* Discard any payload */
    if (frame.len > 0)
    {
        uint8_t *buf = malloc(frame.len + 1);
        if (buf)
        {
            frame.payload = buf;
            httpd_ws_recv_frame(req, &frame, frame.len);
            free(buf);
        }
    }

    return ESP_OK;
}

static int cmd_ws(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("WebSocket clients: %u\n", (unsigned)websocket_client_count());
    return 0;
}

esp_err_t websocket_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex)
    {
        return ESP_ERR_NO_MEM;
    }

    httpd_handle_t server = http_server_get_handle();
    if (!server)
    {
        ESP_LOGE(TAG, "HTTP server not running");
        return ESP_ERR_INVALID_STATE;
    }

    const httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = NULL,
        .is_websocket = true,
    };
    httpd_register_uri_handler(server, &ws_uri);

    const esp_console_cmd_t cmd = {
        .command = "ws",
        .help = "Show WebSocket client count",
        .hint = NULL,
        .func = &cmd_ws,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    ESP_LOGI(TAG, "WebSocket endpoint registered at /ws");
    return ESP_OK;
}
