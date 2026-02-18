#include "filesystem.h"
#include "http_server.h"

#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "http_api";

static esp_err_t handler_files(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }

    char query[256] = {0};
    char path[VFS_PATH_MAX] = "/flash";

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        httpd_query_key_value(query, "path", path, sizeof(path));
    }

    vfs_dir_entry_t entries[VFS_ENTRIES_MAX];
    size_t count = 0;
    esp_err_t err = vfs_list_dir(path, entries, VFS_ENTRIES_MAX, &count);
    if (err != ESP_OK)
    {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_status(req, "400 Bad Request");
        char errbuf[64];
        snprintf(errbuf, sizeof(errbuf), "{\"error\":\"%s\"}", esp_err_to_name(err));
        return httpd_resp_send(req, errbuf, HTTPD_RESP_USE_STRLEN);
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "[");

    for (size_t i = 0; i < count; i++)
    {
        char item[256];
        snprintf(item, sizeof(item), "%s{\"name\":\"%s\",\"size\":%u,\"is_dir\":%s}", (i > 0) ? "," : "",
                 entries[i].name, (unsigned)entries[i].size, entries[i].is_dir ? "true" : "false");
        httpd_resp_sendstr_chunk(req, item);
    }

    httpd_resp_sendstr_chunk(req, "]");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t handler_heap(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%u", (unsigned)esp_get_free_heap_size());
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
}

void http_api_register(httpd_handle_t server)
{
    const httpd_uri_t files_uri = {
        .uri = "/api/files",
        .method = HTTP_GET,
        .handler = handler_files,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &files_uri);

    const httpd_uri_t heap_uri = {
        .uri = "/api/heap",
        .method = HTTP_GET,
        .handler = handler_heap,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &heap_uri);

    ESP_LOGI(TAG, "API endpoints registered");
}
