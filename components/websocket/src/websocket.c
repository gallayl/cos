#include "websocket.h"
#include "http_server.h"

#include "boot_profile.h"
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
static bool s_running = false;
static bool s_commands_registered = false;

static void websocket_pre_stop_hook(void);

void websocket_set_text_handler(websocket_text_handler_t handler, void *context)
{
    if (s_mutex == NULL)
    {
        s_mutex = xSemaphoreCreateMutex();
    }
    if (s_mutex != NULL)
    {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
    }
    s_text_handler = handler;
    s_text_handler_context = context;
    if (s_mutex != NULL)
    {
        xSemaphoreGive(s_mutex);
    }
}

static void remove_client_locked(int fd)
{
    for (size_t i = 0; i < s_client_count; i++)
    {
        if (s_clients[i] == fd)
        {
            s_clients[i] = s_clients[s_client_count - 1];
            s_client_count--;
            break;
        }
    }
}

static void remove_client(int fd)
{
    if (s_mutex == NULL)
    {
        return;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    remove_client_locked(fd);
    xSemaphoreGive(s_mutex);
}

static void add_client(int fd)
{
    if (s_mutex == NULL)
    {
        return;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (!s_running)
    {
        xSemaphoreGive(s_mutex);
        return;
    }
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

void websocket_on_session_close(int sockfd)
{
    remove_client(sockfd);
}

size_t websocket_client_count(void)
{
    if (s_mutex == NULL)
    {
        return 0;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    const size_t count = s_running ? s_client_count : 0;
    xSemaphoreGive(s_mutex);
    return count;
}

bool websocket_is_running(void)
{
    return s_running;
}

esp_err_t websocket_broadcast(const char *msg, size_t len)
{
    httpd_handle_t server = http_server_get_handle();
    if (!server || !msg || len == 0 || !s_running)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int fds[WEBSOCKET_MAX_CLIENTS];
    size_t count = 0;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (!s_running)
    {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }
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
    if (!s_running)
    {
        return ESP_ERR_INVALID_STATE;
    }

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

    websocket_text_handler_t handler = NULL;
    void *handler_ctx = NULL;
    if (s_mutex != NULL)
    {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        handler = s_running ? s_text_handler : NULL;
        handler_ctx = s_text_handler_context;
        xSemaphoreGive(s_mutex);
    }

    if (frame.type == HTTPD_WS_TYPE_TEXT && handler != NULL)
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
            handler(line, len, handler_ctx);
        }
    }

    free(buf);
    return ESP_OK;
}

static int cmd_ws(int argc, char **argv)
{
    if (argc == 1 || (argc >= 2 && strcmp(argv[1], "status") == 0))
    {
        printf("WebSocket: %s\n", websocket_is_running() ? "running" : "stopped");
        printf("Clients:   %u\n", (unsigned)websocket_client_count());
        return 0;
    }

    if (strcmp(argv[1], "start") == 0)
    {
        if (!boot_profile_allows_websocket())
        {
            printf("ws: start failed: forced off by boot profile for this boot\n");
            return 1;
        }
        if (!http_server_is_running())
        {
            printf("ws: start failed: HTTP server is not running\n");
            return 1;
        }
        if (websocket_is_running())
        {
            printf("WebSocket already running\n");
            return 0;
        }
        esp_err_t err = websocket_start();
        if (err != ESP_OK)
        {
            printf("ws: start failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("WebSocket started\n");
        return 0;
    }

    if (strcmp(argv[1], "stop") == 0)
    {
        if (!websocket_is_running())
        {
            printf("WebSocket already stopped\n");
            return 0;
        }
        esp_err_t err = websocket_stop();
        if (err != ESP_OK)
        {
            printf("ws: stop failed (%s)\n", esp_err_to_name(err));
            return 1;
        }
        printf("WebSocket stopped\n");
        return 0;
    }

    printf("Usage: ws [status|start|stop]\n");
    return 1;
}

static void websocket_pre_stop_hook(void)
{
    /* Best-effort dependency stop: WebSocket must not outlive HTTP server. */
    (void)websocket_stop();
}

esp_err_t websocket_start(void)
{
    if (s_running)
    {
        return ESP_OK;
    }

    httpd_handle_t server = http_server_get_handle();
    if (!http_server_is_running() || server == NULL)
    {
        ESP_LOGE(TAG, "HTTP server not running");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_mutex == NULL)
    {
        s_mutex = xSemaphoreCreateMutex();
        if (s_mutex == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    if (!s_commands_registered)
    {
        const esp_console_cmd_t cmd = {
            .command = "ws",
            .help = "WebSocket service control (status, start, stop)",
            .hint = "[status|start|stop]",
            .func = &cmd_ws,
        };
        esp_err_t cmd_err = esp_console_cmd_register(&cmd);
        if (cmd_err == ESP_OK)
        {
            s_commands_registered = true;
            ESP_LOGI(TAG, "Registered 'ws' command");
        }
        else if (cmd_err == ESP_ERR_INVALID_STATE)
        {
            s_commands_registered = true;
            ESP_LOGD(TAG, "'ws' command already registered");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to register 'ws' command: %s", esp_err_to_name(cmd_err));
            return cmd_err;
        }
    }

    /* Reset runtime client state; configured text handler is preserved and becomes active when running. */
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_client_count = 0;
    xSemaphoreGive(s_mutex);

    http_server_set_session_close_cb(websocket_on_session_close);
    http_server_set_pre_stop_hook(websocket_pre_stop_hook);

    const httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = NULL,
        .is_websocket = true,
    };
    esp_err_t err = httpd_register_uri_handler(server, &ws_uri);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register /ws handler: %s", esp_err_to_name(err));
        http_server_clear_session_close_cb_if(websocket_on_session_close);
        return err;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_running = true;
    xSemaphoreGive(s_mutex);

    ESP_LOGI(TAG, "WebSocket endpoint registered at /ws");
    return ESP_OK;
}

esp_err_t websocket_stop(void)
{
    if (!s_running)
    {
        return ESP_OK;
    }

    /* Mark stopped first to prevent dispatch/broadcasts using stale state. */
    if (s_mutex != NULL)
    {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        s_running = false;
        s_client_count = 0;
        xSemaphoreGive(s_mutex);
    }
    else
    {
        s_running = false;
        s_client_count = 0;
    }

    httpd_handle_t server = http_server_get_handle();
    if (server != NULL && http_server_is_running())
    {
        /* Best-effort: unregister endpoint so new upgrades/frames are not accepted. */
        (void)httpd_unregister_uri_handler(server, "/ws", HTTP_GET);
    }

    http_server_clear_session_close_cb_if(websocket_on_session_close);
    /* Pre-stop hook may remain registered; it is harmless (idempotent stop). */

    ESP_LOGI(TAG, "WebSocket stopped");
    return ESP_OK;
}

esp_err_t websocket_init(void)
{
    return websocket_start();
}
