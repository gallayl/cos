#include "websocket.h"
#include "http_server.h"

#include "esp_console.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "websocket";

static int s_clients[WEBSOCKET_MAX_CLIENTS];
static size_t s_client_count = 0;
static SemaphoreHandle_t s_mutex = NULL;
static websocket_text_handler_t s_text_handler = NULL;
static void *s_text_handler_context = NULL;

void websocket_set_text_handler(websocket_text_handler_t handler, void *context)
{
    s_text_handler = handler;
    s_text_handler_context = context;
}

static void remove_client(int fd);

static void add_client(int fd)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    bool known = false;
    for (size_t i = 0; i < s_client_count; i++)
    {
        if (s_clients[i] == fd)
        {
            known = true;
            break;
        }
    }
    if (!known && s_client_count < WEBSOCKET_MAX_CLIENTS)
    {
        s_clients[s_client_count++] = fd;
    }
    xSemaphoreGive(s_mutex);
}

typedef struct
{
    uint8_t *payload;
} ws_tx_t;

static void ws_tx_done(esp_err_t err, int sockfd, void *arg)
{
    ws_tx_t *tx = (ws_tx_t *)arg;
    if (err != ESP_OK)
    {
        remove_client(sockfd);
    }
    free(tx->payload);
    free(tx);
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
    if (!server || !msg || len == 0)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int fds[WEBSOCKET_MAX_CLIENTS];
    size_t count = 0;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    count = s_client_count;
    for (size_t i = 0; i < count; i++)
    {
        fds[i] = s_clients[i];
    }
    xSemaphoreGive(s_mutex);

    /* Queue onto the HTTP server task. send_frame is not safe from the shell task,
       which is where console output is printed. */
    for (size_t i = 0; i < count; i++)
    {
        uint8_t *payload = malloc(len);
        ws_tx_t *tx = malloc(sizeof(*tx));
        if (payload == NULL || tx == NULL)
        {
            free(payload);
            free(tx);
            return ESP_ERR_NO_MEM;
        }
        memcpy(payload, msg, len);
        tx->payload = payload;

        httpd_ws_frame_t frame = {
            .type = HTTPD_WS_TYPE_TEXT,
            .payload = payload,
            .len = len,
        };
        if (httpd_ws_send_data_async(server, fds[i], &frame, ws_tx_done, tx) != ESP_OK)
        {
            free(payload);
            free(tx);
            remove_client(fds[i]);
        }
    }

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

    httpd_ws_frame_t frame = {.final = false};
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

    if (frame.len == 0)
    {
        return ESP_OK;
    }

    if (frame.len > WEBSOCKET_MAX_TEXT)
    {
        ESP_LOGW(TAG, "Dropping %u byte frame", (unsigned)frame.len);
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t *buf = malloc(frame.len);
    if (buf == NULL)
    {
        ESP_LOGW(TAG, "Dropping %u byte frame", (unsigned)frame.len);
        return ESP_ERR_NO_MEM;
    }

    frame.payload = buf;
    ret = httpd_ws_recv_frame(req, &frame, frame.len);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to receive frame payload: %s", esp_err_to_name(ret));
        free(buf);
        return ret;
    }

    if (frame.type == HTTPD_WS_TYPE_TEXT)
    {
        add_client(httpd_req_to_sockfd(req));
    }

    if (frame.type == HTTPD_WS_TYPE_TEXT && s_text_handler != NULL)
    {
        char line[WEBSOCKET_MAX_TEXT + 1];
        size_t len = frame.len;
        memcpy(line, buf, len);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            len--;
        }
        if (len > 0)
        {
            line[len++] = '\n';
            s_text_handler(line, len, s_text_handler_context);
        }
    }

    free(buf);
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
    httpd_handle_t server = http_server_get_handle();
    if (!server)
    {
        ESP_LOGE(TAG, "HTTP server not running");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_mutex == NULL)
    {
        s_mutex = xSemaphoreCreateMutex();
        if (!s_mutex)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    http_server_set_session_close_cb(websocket_on_session_close);

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
