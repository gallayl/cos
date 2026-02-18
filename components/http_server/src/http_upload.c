#include "filesystem.h"
#include "http_multipart.h"
#include "http_server.h"

#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *const TAG = "http_upload";

typedef struct
{
    char target_path[VFS_PATH_MAX];
    bool has_path;
    FILE *file;
    char error[128];
} upload_ctx_t;

static void upload_field_cb(const char *name, const char *value, void *ctx)
{
    upload_ctx_t *uctx = (upload_ctx_t *)ctx;
    if (strcmp(name, "path") == 0)
    {
        char sanitized[VFS_PATH_MAX];
        if (http_sanitize_upload_path(value, sanitized, sizeof(sanitized)) == ESP_OK)
        {
            strncpy(uctx->target_path, sanitized, sizeof(uctx->target_path) - 1);
            uctx->has_path = true;
        }
        else
        {
            snprintf(uctx->error, sizeof(uctx->error), "Invalid path");
        }
    }
}

static bool upload_file_cb(const char *field_name, const char *file_name, const uint8_t *data, size_t len,
                           bool is_first, bool is_final, void *ctx)
{
    (void)field_name;
    upload_ctx_t *uctx = (upload_ctx_t *)ctx;

    if (is_first && !uctx->has_path)
    {
        snprintf(uctx->error, sizeof(uctx->error), "Missing path field");
        return false;
    }

    if (is_first)
    {
        /* Resolve virtual path to real path */
        char real_path[VFS_PATH_MAX];
        if (vfs_resolve_path(uctx->target_path, real_path, sizeof(real_path)) != ESP_OK)
        {
            snprintf(uctx->error, sizeof(uctx->error), "Path resolution failed");
            return false;
        }
        strncpy(uctx->target_path, real_path, sizeof(uctx->target_path) - 1);

        /* Create parent directory */
        char parent[VFS_PATH_MAX];
        strncpy(parent, uctx->target_path, sizeof(parent) - 1);
        parent[sizeof(parent) - 1] = '\0';
        char *last_slash = strrchr(parent, '/');
        if (last_slash && last_slash != parent)
        {
            *last_slash = '\0';
            vfs_mkdir(parent);
        }

        uctx->file = fopen(uctx->target_path, "wb");
        if (!uctx->file)
        {
            snprintf(uctx->error, sizeof(uctx->error), "Failed to open file");
            return false;
        }
    }

    if (data && len > 0 && uctx->file)
    {
        if (fwrite(data, 1, len, uctx->file) != len)
        {
            snprintf(uctx->error, sizeof(uctx->error), "Write failed");
            fclose(uctx->file);
            uctx->file = NULL;
            return false;
        }
    }

    if (is_final && uctx->file)
    {
        fclose(uctx->file);
        uctx->file = NULL;
        ESP_LOGI(TAG, "Upload complete: %s (%s)", uctx->target_path, file_name);
    }
    return true;
}

static esp_err_t handler_upload(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }

    upload_ctx_t uctx = {0};

    esp_err_t ret = http_multipart_parse(req, upload_field_cb, upload_file_cb, &uctx);

    if (uctx.file)
    {
        fclose(uctx.file);
        uctx.file = NULL;
    }

    httpd_resp_set_type(req, "application/json");

    if (uctx.error[0] != '\0')
    {
        httpd_resp_set_status(req, "400 Bad Request");
        char resp[192];
        snprintf(resp, sizeof(resp), "{\"error\":\"%s\"}", uctx.error);
        return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    }

    if (ret != ESP_OK)
    {
        httpd_resp_set_status(req, "500 Internal Server Error");
        return httpd_resp_send(req, "{\"error\":\"Upload failed\"}", HTTPD_RESP_USE_STRLEN);
    }

    return httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
}

void http_upload_register(httpd_handle_t server)
{
    const httpd_uri_t uri = {
        .uri = "/api/upload",
        .method = HTTP_POST,
        .handler = handler_upload,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &uri);
    ESP_LOGI(TAG, "Upload endpoint registered");
}
