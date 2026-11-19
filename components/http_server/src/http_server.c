#include "http_server.h"
#include "http_auth.h"

#include "esp_log.h"

#include <unistd.h>

static const char *const TAG = "http_server";

static httpd_handle_t s_server = NULL;
static http_session_close_cb_t s_session_close_cb = NULL;
static void (*s_pre_stop_hook)(void) = NULL;
static bool s_commands_registered = false;

void http_server_set_session_close_cb(http_session_close_cb_t cb)
{
    s_session_close_cb = cb;
}

void http_server_clear_session_close_cb_if(http_session_close_cb_t cb)
{
    if (s_session_close_cb == cb)
    {
        s_session_close_cb = NULL;
    }
}

void http_server_set_pre_stop_hook(void (*hook)(void))
{
    s_pre_stop_hook = hook;
}

static void on_session_close(httpd_handle_t hd, int sockfd)
{
    (void)hd;
    http_session_close_cb_t cb = s_session_close_cb;
    if (cb != NULL)
    {
        cb(sockfd);
    }
    close(sockfd);
}

/* Forward declarations for sub-module registration */
void http_static_init(void);
esp_err_t http_static_handler(httpd_req_t *req, httpd_err_code_t err_code);
void http_api_register(httpd_handle_t server);
void http_upload_register(httpd_handle_t server);
void http_server_register_commands(void);

httpd_handle_t http_server_get_handle(void)
{
    return s_server;
}

bool http_server_is_running(void)
{
    return s_server != NULL;
}

esp_err_t http_server_stop(void)
{
    if (s_server == NULL)
    {
        return ESP_OK;
    }

    void (*pre_stop_hook)(void) = s_pre_stop_hook;
    if (pre_stop_hook != NULL)
    {
        pre_stop_hook();
    }

    httpd_handle_t server = s_server;
    s_server = NULL;
    /* Best-effort: stop accepting/serving and teardown httpd resources.
       Session-close callbacks may run during stop; callback pointer remains readable. */
    esp_err_t err = httpd_stop(server);
    s_session_close_cb = NULL;

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "httpd_stop failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "HTTP server stopped");
    return ESP_OK;
}

esp_err_t http_server_start(void)
{
    if (s_server != NULL)
    {
        return ESP_OK;
    }

    http_auth_init();
    http_static_init();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.lru_purge_enable = true;
    config.stack_size = 8192;
    /* Keep max_open_sockets within LWIP/httpd limits (IDF v6.1 reserves 3 internally).
       Increasing it requires CONFIG_LWIP_MAX_SOCKETS; default 7 gives 4 client slots. */
    config.max_open_sockets = 7;
    config.close_fn = on_session_close;

    httpd_handle_t server = NULL;
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(err));
        return err;
    }

    s_server = server;

    /* Error/static handler registration and endpoint registration are best-effort in terms of
       individual call error handling (ESP-IDF APIs return esp_err_t but existing code ignored them);
       preserve existing behavior while ensuring handlers are registered on the fresh server. */
    (void)httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_static_handler);
    http_api_register(server);
    http_upload_register(server);

    if (!s_commands_registered)
    {
        http_server_register_commands();
        s_commands_registered = true;
    }

    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    return ESP_OK;
}

esp_err_t http_server_init(void)
{
    return http_server_start();
}
