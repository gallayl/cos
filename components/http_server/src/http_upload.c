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
    }

    const char *mode = is_first ? "w" : "a";
    FILE *f = fopen(uctx->target_path, mode);
    if (!f)
    {
        snprintf(uctx->error, sizeof(uctx->error), "Failed to open file");
        return false;
    }
    if (data && len > 0)
    {
        fwrite(data, 1, len, f);
    }
    fclose(f);

    if (is_final)
    {
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

/* --- Generic upload handler registration (for OTA) --- */

typedef struct
{
    http_upload_chunk_cb on_chunk;
    void *user_ctx;
} generic_upload_ctx_t;

static generic_upload_ctx_t s_upload_handlers[4];
static int s_upload_handler_count = 0;

static bool generic_file_cb(const char *field_name, const char *file_name, const uint8_t *data, size_t len,
                            bool is_first, bool is_final, void *ctx)
{
    (void)field_name;
    (void)file_name;
    generic_upload_ctx_t *gctx = (generic_upload_ctx_t *)ctx;
    return gctx->on_chunk(data, len, is_first, is_final, gctx->user_ctx) == ESP_OK;
}

static esp_err_t generic_upload_handler(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }

    generic_upload_ctx_t *gctx = (generic_upload_ctx_t *)req->user_ctx;
    return http_multipart_parse(req, NULL, generic_file_cb, gctx);
}

esp_err_t http_register_upload_handler(const char *uri, http_upload_chunk_cb on_chunk, void *ctx)
{
    httpd_handle_t server = http_server_get_handle();
    if (!server || !uri || !on_chunk)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_upload_handler_count >= 4)
    {
        return ESP_ERR_NO_MEM;
    }

    generic_upload_ctx_t *gctx = &s_upload_handlers[s_upload_handler_count++];
    gctx->on_chunk = on_chunk;
    gctx->user_ctx = ctx;

    const httpd_uri_t handler = {
        .uri = uri,
        .method = HTTP_POST,
        .handler = generic_upload_handler,
        .user_ctx = gctx,
    };
    return httpd_register_uri_handler(server, &handler);
}
