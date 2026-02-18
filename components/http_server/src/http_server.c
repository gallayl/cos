#include "http_server.h"
#include "http_auth.h"

#include "esp_log.h"

static const char *const TAG = "http_server";

static httpd_handle_t s_server = NULL;

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

esp_err_t http_server_stop(void)
{
    if (s_server)
    {
        httpd_stop(s_server);
        s_server = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
    return ESP_OK;
}

esp_err_t http_server_init(void)
{
    http_auth_init();
    http_static_init();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.lru_purge_enable = true;
    config.stack_size = 8192;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(err));
        return err;
    }

    httpd_register_err_handler(s_server, HTTPD_404_NOT_FOUND, http_static_handler);

    http_api_register(s_server);
    http_upload_register(s_server);
    http_server_register_commands();

    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    return ESP_OK;
}
