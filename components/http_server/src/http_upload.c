#include "filesystem.h"
#include "http_multipart.h"
#include "http_server.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char *const TAG = "http_upload";

static void upload_log_measure(const char *event, size_t content_len, esp_err_t result)
{
    const uint32_t free_heap = esp_get_free_heap_size();
    const size_t largest_8bit = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    ESP_LOGI(TAG, "upload-measure: %s free=%u largest_8bit=%u content_len=%u result=%s", event, (unsigned)free_heap,
             (unsigned)largest_8bit, (unsigned)content_len, esp_err_to_name(result));
}

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
            snprintf(uctx->target_path, sizeof(uctx->target_path), "%.*s", (int)(sizeof(uctx->target_path) - 1),
                     sanitized);
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
    (void)file_name;
    upload_ctx_t *uctx = (upload_ctx_t *)ctx;

    if (is_first && !uctx->has_path) // NOLINT(readability-implicit-bool-conversion)
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
        snprintf(uctx->target_path, sizeof(uctx->target_path), "%.*s", (int)(sizeof(uctx->target_path) - 1), real_path);

        /* Create parent directories on the resolved real path. */
        char parent[VFS_PATH_MAX];
        strncpy(parent, uctx->target_path, sizeof(parent) - 1);
        parent[sizeof(parent) - 1] = '\0';
        char *last_slash = strrchr(parent, '/');
        if (last_slash && last_slash != parent)
        {
            *last_slash = '\0';
            for (char *p = parent + 1; *p != '\0'; p++)
            {
                if (*p == '/')
                {
                    *p = '\0';
                    struct stat st;
                    if (stat(parent, &st) != 0)
                    {
                        if (mkdir(parent, 0775) != 0)
                        {
                            if (errno != EEXIST || stat(parent, &st) != 0 || !S_ISDIR(st.st_mode))
                            {
                                snprintf(uctx->error, sizeof(uctx->error), "Failed to create directory");
                                return false;
                            }
                        }
                    }
                    else if (!S_ISDIR(st.st_mode))
                    {
                        snprintf(uctx->error, sizeof(uctx->error), "Failed to create directory");
                        return false;
                    }
                    *p = '/';
                }
            }

            struct stat st;
            if (stat(parent, &st) != 0)
            {
                if (mkdir(parent, 0775) != 0)
                {
                    if (errno != EEXIST || stat(parent, &st) != 0 || !S_ISDIR(st.st_mode))
                    {
                        snprintf(uctx->error, sizeof(uctx->error), "Failed to create directory");
                        return false;
                    }
                }
            }
            else if (!S_ISDIR(st.st_mode))
            {
                snprintf(uctx->error, sizeof(uctx->error), "Failed to create directory");
                return false;
            }
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

    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
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

    const size_t content_len = req ? req->content_len : 0;
    upload_log_measure("handler_start", content_len, ESP_OK);

    upload_ctx_t uctx = {0};

    esp_err_t ret = http_multipart_parse(req, upload_field_cb, upload_file_cb, &uctx);

    if (uctx.file)
    {
        fclose(uctx.file);
        uctx.file = NULL;
    }

    upload_log_measure("parse_return", content_len, ret);

    httpd_resp_set_type(req, "application/json");

    if (uctx.error[0] != '\0')
    {
        esp_err_t log_result = ESP_FAIL;
        if (strcmp(uctx.error, "Invalid path") == 0 || strcmp(uctx.error, "Missing path field") == 0)
        {
            log_result = ESP_ERR_INVALID_ARG;
        }

        httpd_resp_set_status(req, "400 Bad Request");
        char resp[192];
        snprintf(resp, sizeof(resp), "{\"error\":\"%s\"}", uctx.error);
        upload_log_measure("handler_error_response", content_len, log_result);
        return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    }

    if (ret != ESP_OK)
    {
        const char *name = esp_err_to_name(ret);
        ESP_LOGW(TAG, "Upload parse error: %s", name);
        httpd_resp_set_status(req, "500 Internal Server Error");
        if (ret == ESP_ERR_NO_MEM)
        {
            upload_log_measure("handler_parse_nomem", content_len, ret);
            return httpd_resp_send(req, "{\"error\":\"Upload failed: out of memory\"}", HTTPD_RESP_USE_STRLEN);
        }
        if (ret == ESP_ERR_INVALID_ARG)
        {
            upload_log_measure("handler_parse_invalid_arg", content_len, ret);
            return httpd_resp_send(req, "{\"error\":\"Upload failed: invalid request\"}", HTTPD_RESP_USE_STRLEN);
        }
        if (ret == ESP_ERR_INVALID_STATE)
        {
            upload_log_measure("handler_parse_invalid_state", content_len, ret);
            return httpd_resp_send(req, "{\"error\":\"Upload failed: incomplete request\"}", HTTPD_RESP_USE_STRLEN);
        }
        char resp[96];
        snprintf(resp, sizeof(resp), "{\"error\":\"Upload failed: %s\"}", name);
        upload_log_measure("handler_parse_error", content_len, ret);
        return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    }

    upload_log_measure("handler_success", content_len, ret);
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
