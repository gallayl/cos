#include "filesystem.h"
#include "http_server.h"

#include "esp_log.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TAG = "http_api";

static esp_err_t send_api_error(httpd_req_t *req, const char *status, const char *message);

static esp_err_t path_from_query(const char *raw_path, char *path, size_t path_len, bool allow_root)
{
    char decoded[VFS_PATH_MAX];
    esp_err_t err = http_url_decode(raw_path, decoded, sizeof(decoded));
    if (err != ESP_OK)
    {
        return err;
    }

    if (allow_root && strcmp(decoded, "/") == 0)
    {
        if (path_len < 2)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        path[0] = '/';
        path[1] = '\0';
        return ESP_OK;
    }

    return http_sanitize_upload_path(decoded, path, path_len);
}

static size_t json_escape(const char *src, char *dst, size_t dst_len)
{
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < dst_len - 1; i++)
    {
        char c = src[i];
        if (c == '"' || c == '\\')
        {
            if (j + 2 >= dst_len)
            {
                break;
            }
            dst[j++] = '\\';
        }
        dst[j++] = c;
    }
    dst[j] = '\0';
    return j;
}

static esp_err_t handler_files(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }

    char query[512] = {0};
    char raw_path[384] = {0};
    char path[VFS_PATH_MAX] = "/flash";

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
        httpd_query_key_value(query, "path", raw_path, sizeof(raw_path)) == ESP_OK)
    {
        if (path_from_query(raw_path, path, sizeof(path), true) != ESP_OK)
        {
            return send_api_error(req, "400 Bad Request", "Invalid path");
        }
    }

    vfs_dir_entry_t *entries = malloc(VFS_ENTRIES_MAX * sizeof(vfs_dir_entry_t));
    if (entries == NULL)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_OK;
    }

    size_t count = 0;
    esp_err_t err = vfs_list_dir(path, entries, VFS_ENTRIES_MAX, &count);
    if (err != ESP_OK)
    {
        free(entries);
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
        char escaped_name[128];
        json_escape(entries[i].name, escaped_name, sizeof(escaped_name));
        char item[320];
        snprintf(item, sizeof(item),
                 "%s{\"name\":\"%s\",\"size\":%u,\"is_dir\":%s,\"mtime\":%lld,\"ctime\":%lld}", (i > 0) ? "," : "",
                 escaped_name, (unsigned)entries[i].size, entries[i].is_dir ? "true" : "false",
                 (long long)entries[i].mtime, (long long)entries[i].ctime);
        httpd_resp_sendstr_chunk(req, item);
    }

    free(entries);
    httpd_resp_sendstr_chunk(req, "]");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t send_api_error(httpd_req_t *req, const char *status, const char *message)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, status);
    char errbuf[96];
    snprintf(errbuf, sizeof(errbuf), "{\"error\":\"%s\"}", message);
    return httpd_resp_send(req, errbuf, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stream_file_chunk(const void *data, size_t len, void *context)
{
    httpd_req_t *req = (httpd_req_t *)context;
    return httpd_resp_send_chunk(req, (const char *)data, (ssize_t)len);
}

static esp_err_t handler_file(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }

    char query[512] = {0};
    char raw_path[384] = {0};
    char path[VFS_PATH_MAX];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "path", raw_path, sizeof(raw_path)) != ESP_OK)
    {
        return send_api_error(req, "400 Bad Request", "Missing path");
    }

    if (path_from_query(raw_path, path, sizeof(path), false) != ESP_OK)
    {
        return send_api_error(req, "400 Bad Request", "Invalid path");
    }

    if (!vfs_exists(path) || vfs_is_directory(path))
    {
        return send_api_error(req, "404 Not Found", "Not found");
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    const esp_err_t err = vfs_read_file_stream(path, stream_file_chunk, req);
    const esp_err_t end = httpd_resp_send_chunk(req, NULL, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(err));
    }
    return end;
}

static bool is_filesystem_root(const char *path)
{
    return strcmp(path, "/flash") == 0 || strcmp(path, "/sdcard") == 0;
}

static bool path_is_inside(const char *parent, const char *child)
{
    size_t length = strlen(parent);
    return strncmp(child, parent, length) == 0 && child[length] == '/';
}

static esp_err_t query_path(httpd_req_t *req, const char *key, char *path, size_t path_len)
{
    char query[1024] = {0};
    char raw[384] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, key, raw, sizeof(raw)) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return path_from_query(raw, path, path_len, false);
}

static esp_err_t send_ok(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t handler_mkdir(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }

    char path[VFS_PATH_MAX];
    if (query_path(req, "path", path, sizeof(path)) != ESP_OK || is_filesystem_root(path))
    {
        return send_api_error(req, "400 Bad Request", "Invalid path");
    }
    if (vfs_exists(path))
    {
        return send_api_error(req, "400 Bad Request", "Already exists");
    }
    if (vfs_mkdir(path) != ESP_OK)
    {
        return send_api_error(req, "400 Bad Request", "Create directory failed");
    }
    return send_ok(req);
}

static esp_err_t handler_rename(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }

    char from[VFS_PATH_MAX];
    char to[VFS_PATH_MAX];
    if (query_path(req, "from", from, sizeof(from)) != ESP_OK || query_path(req, "to", to, sizeof(to)) != ESP_OK ||
        is_filesystem_root(from) || is_filesystem_root(to) || path_is_inside(from, to))
    {
        return send_api_error(req, "400 Bad Request", "Invalid path");
    }

    esp_err_t err = vfs_rename_file(from, to);
    if (err == ESP_ERR_NOT_FOUND)
    {
        return send_api_error(req, "404 Not Found", "Not found");
    }
    if (err == ESP_ERR_INVALID_STATE)
    {
        return send_api_error(req, "400 Bad Request", "Already exists");
    }
    if (err == ESP_ERR_NOT_SUPPORTED)
    {
        return send_api_error(req, "400 Bad Request", "Paths must be on the same filesystem");
    }
    if (err != ESP_OK)
    {
        return send_api_error(req, "400 Bad Request", "Rename failed");
    }
    return send_ok(req);
}

static esp_err_t handler_delete(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }

    char path[VFS_PATH_MAX];
    if (query_path(req, "path", path, sizeof(path)) != ESP_OK || is_filesystem_root(path))
    {
        return send_api_error(req, "400 Bad Request", "Invalid path");
    }
    if (!vfs_exists(path))
    {
        return send_api_error(req, "404 Not Found", "Not found");
    }
    if (vfs_remove(path) != ESP_OK)
    {
        return send_api_error(req, "400 Bad Request", "Remove failed");
    }
    return send_ok(req);
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

    const httpd_uri_t file_uri = {
        .uri = "/api/file",
        .method = HTTP_GET,
        .handler = handler_file,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &file_uri);

    const httpd_uri_t delete_uri = {
        .uri = "/api/file",
        .method = HTTP_DELETE,
        .handler = handler_delete,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &delete_uri);

    const httpd_uri_t mkdir_uri = {
        .uri = "/api/mkdir",
        .method = HTTP_POST,
        .handler = handler_mkdir,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &mkdir_uri);

    const httpd_uri_t rename_uri = {
        .uri = "/api/rename",
        .method = HTTP_POST,
        .handler = handler_rename,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &rename_uri);

    const httpd_uri_t heap_uri = {
        .uri = "/api/heap",
        .method = HTTP_GET,
        .handler = handler_heap,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &heap_uri);

    ESP_LOGI(TAG, "API endpoints registered");
}
