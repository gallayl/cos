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
#include <unistd.h>

static const char *const TAG = "http_upload";

#define UPLOAD_MAX_BYTES 65536U
#define UPLOAD_MAX_CONTENT_LEN (UPLOAD_MAX_BYTES + 1024U) /* allow multipart overhead for fast precheck */
#define UPLOAD_PREFLIGHT_MIN_LARGEST_8BIT 12288U
#define UPLOAD_TMP_SUFFIX ".tmp"

static void upload_log_measure(const char *event, size_t content_len, esp_err_t result)
{
    const uint32_t free_heap = esp_get_free_heap_size();
    const size_t largest_8bit = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    ESP_LOGI(TAG, "upload-measure: %s free=%u largest_8bit=%u content_len=%u result=%s", event, (unsigned)free_heap,
             (unsigned)largest_8bit, (unsigned)content_len, esp_err_to_name(result));
}

typedef enum
{
    UPLOAD_ERR_NONE = 0,
    UPLOAD_ERR_INVALID_REQUEST, /* malformed/invalid request (locked 400 contract) */
    UPLOAD_ERR_TOO_LARGE,       /* locked 413 contract */
    UPLOAD_ERR_ABORTED,         /* locked 503 abort contract (when response possible) */
    UPLOAD_ERR_LOW_MEMORY,      /* locked 503 preflight contract */
    UPLOAD_ERR_INTERNAL,        /* server/filesystem/write failures */
} upload_error_kind_t;

typedef struct
{
    char target_path[VFS_PATH_MAX];
    char temp_path[VFS_PATH_MAX + 8];
    bool has_path;
    bool temp_opened;
    bool file_part_seen;
    size_t file_bytes_written;
    FILE *file;
    upload_error_kind_t err_kind;
} upload_ctx_t;

static void upload_cleanup_ctx(upload_ctx_t *uctx)
{
    if (uctx->file)
    {
        fclose(uctx->file);
        uctx->file = NULL;
    }
    if (uctx->temp_opened && uctx->temp_path[0] != '\0')
    {
        unlink(uctx->temp_path);
        uctx->temp_opened = false;
    }
}

static void upload_field_cb(const char *name, const char *value, void *ctx)
{
    upload_ctx_t *uctx = (upload_ctx_t *)ctx;
    if (uctx->err_kind != UPLOAD_ERR_NONE)
    {
        return;
    }
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
            uctx->err_kind = UPLOAD_ERR_INVALID_REQUEST;
        }
    }
}

static bool upload_ensure_parent_dirs(const char *path, upload_ctx_t *uctx)
{
    char parent[VFS_PATH_MAX];
    strncpy(parent, path, sizeof(parent) - 1);
    parent[sizeof(parent) - 1] = '\0';
    char *last_slash = strrchr(parent, '/');
    if (!last_slash || last_slash == parent)
    {
        return true;
    }

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
                        uctx->err_kind = UPLOAD_ERR_INTERNAL;
                        return false;
                    }
                }
            }
            else if (!S_ISDIR(st.st_mode))
            {
                uctx->err_kind = UPLOAD_ERR_INTERNAL;
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
                uctx->err_kind = UPLOAD_ERR_INTERNAL;
                return false;
            }
        }
    }
    else if (!S_ISDIR(st.st_mode))
    {
        uctx->err_kind = UPLOAD_ERR_INTERNAL;
        return false;
    }

    return true;
}

static bool upload_file_cb(const char *field_name, const char *file_name, const uint8_t *data, size_t len,
                           bool is_first, bool is_final, void *ctx)
{
    (void)field_name;
    (void)file_name;
    upload_ctx_t *uctx = (upload_ctx_t *)ctx;

    if (uctx->err_kind != UPLOAD_ERR_NONE)
    {
        return false;
    }

    if (is_first && !uctx->has_path) // NOLINT(readability-implicit-bool-conversion)
    {
        uctx->err_kind = UPLOAD_ERR_INVALID_REQUEST;
        return false;
    }

    if (is_first)
    {
        /* Single-file uploads only (< 64 KB): reject additional/unsupported file parts. */
        if (uctx->file_part_seen || uctx->temp_opened)
        {
            uctx->err_kind = UPLOAD_ERR_TOO_LARGE;
            return false;
        }
        uctx->file_part_seen = true;
        char real_path[VFS_PATH_MAX];
        if (vfs_resolve_path(uctx->target_path, real_path, sizeof(real_path)) != ESP_OK)
        {
            uctx->err_kind = UPLOAD_ERR_INTERNAL;
            return false;
        }
        snprintf(uctx->target_path, sizeof(uctx->target_path), "%.*s", (int)(sizeof(uctx->target_path) - 1), real_path);

        if (!upload_ensure_parent_dirs(uctx->target_path, uctx))
        {
            return false;
        }

        int n = snprintf(uctx->temp_path, sizeof(uctx->temp_path), "%s%s", uctx->target_path, UPLOAD_TMP_SUFFIX);
        if (n <= 0 || (size_t)n >= sizeof(uctx->temp_path))
        {
            uctx->err_kind = UPLOAD_ERR_INTERNAL;
            return false;
        }

        uctx->file = fopen(uctx->temp_path, "wb");
        if (!uctx->file)
        {
            uctx->err_kind = UPLOAD_ERR_INTERNAL;
            return false;
        }
        uctx->temp_opened = true;
    }

    if (data && len > 0 && uctx->file)
    {
        if (uctx->file_bytes_written > UPLOAD_MAX_BYTES || len > UPLOAD_MAX_BYTES - uctx->file_bytes_written)
        {
            uctx->err_kind = UPLOAD_ERR_TOO_LARGE;
            upload_cleanup_ctx(uctx);
            return false;
        }
        if (fwrite(data, 1, len, uctx->file) != len)
        {
            uctx->err_kind = UPLOAD_ERR_INTERNAL;
            upload_cleanup_ctx(uctx);
            return false;
        }
        uctx->file_bytes_written += len;
    }

    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (is_final && uctx->file)
    {
        if (fclose(uctx->file) != 0)
        {
            uctx->file = NULL;
            uctx->err_kind = UPLOAD_ERR_INTERNAL;
            upload_cleanup_ctx(uctx);
            return false;
        }
        uctx->file = NULL;

        if (rename(uctx->temp_path, uctx->target_path) != 0)
        {
            /* FAT/VFS may reject rename-over-existing; try safe replace fallback. */
            unlink(uctx->target_path);
            if (rename(uctx->temp_path, uctx->target_path) != 0)
            {
                uctx->err_kind = UPLOAD_ERR_INTERNAL;
                upload_cleanup_ctx(uctx);
                return false;
            }
        }
        uctx->temp_opened = false;
        ESP_LOGI(TAG, "Upload complete: %s (%s)", uctx->target_path, file_name ? file_name : "");
    }
    return true;
}

static esp_err_t upload_send_json(httpd_req_t *req, const char *status, const char *json, size_t content_len,
                                  const char *event, esp_err_t result)
{
    httpd_resp_set_type(req, "application/json");
    if (status)
    {
        httpd_resp_set_status(req, status);
    }
    esp_err_t send_ret = httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    upload_log_measure(event, content_len, (result != ESP_OK) ? result : send_ret);
    return send_ret;
}

static esp_err_t handler_upload(httpd_req_t *req)
{
    if (http_auth_check(req) != ESP_OK)
    {
        return ESP_OK;
    }

    const size_t content_len = req ? req->content_len : 0;
    upload_log_measure("handler_start", content_len, ESP_OK);

    if (content_len > UPLOAD_MAX_CONTENT_LEN)
    {
        return upload_send_json(req, "413 Payload Too Large",
                                "{\"error\":\"upload_too_large\",\"message\":\"Upload exceeds supported limit\","
                                "\"max_bytes\":65536}",
                                content_len, "handler_reject_too_large", ESP_ERR_INVALID_SIZE);
    }

    const size_t largest_8bit = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    if (largest_8bit < UPLOAD_PREFLIGHT_MIN_LARGEST_8BIT)
    {
        ESP_LOGW(TAG, "Upload preflight rejected: largest_8bit=%u < %u", (unsigned)largest_8bit,
                 (unsigned)UPLOAD_PREFLIGHT_MIN_LARGEST_8BIT);
        return upload_send_json(req, "503 Service Unavailable",
                                "{\"error\":\"upload_rejected_low_memory\","
                                "\"message\":\"Upload rejected: insufficient contiguous memory\"}",
                                content_len, "handler_reject_low_memory", ESP_ERR_NO_MEM);
    }

    upload_ctx_t uctx = {0};

    esp_err_t ret = http_multipart_parse(req, upload_field_cb, upload_file_cb, &uctx);

    upload_cleanup_ctx(&uctx);
    upload_log_measure("parse_return", content_len, ret);

    if (uctx.err_kind == UPLOAD_ERR_TOO_LARGE)
    {
        return upload_send_json(req, "413 Payload Too Large",
                                "{\"error\":\"upload_too_large\",\"message\":\"Upload exceeds supported limit\","
                                "\"max_bytes\":65536}",
                                content_len, "handler_ctx_too_large", ESP_ERR_INVALID_SIZE);
    }

    if (uctx.err_kind == UPLOAD_ERR_INVALID_REQUEST)
    {
        return upload_send_json(
            req, "400 Bad Request",
            "{\"error\":\"upload_invalid_request\",\"message\":\"Upload request is malformed or invalid\"}",
            content_len, "handler_ctx_invalid_request", ESP_ERR_INVALID_ARG);
    }

    if (uctx.err_kind == UPLOAD_ERR_ABORTED)
    {
        return upload_send_json(req, "503 Service Unavailable",
                                "{\"error\":\"upload_aborted\",\"message\":\"Upload aborted\"}", content_len,
                                "handler_ctx_aborted", ESP_FAIL);
    }

    if (uctx.err_kind == UPLOAD_ERR_INTERNAL)
    {
        return upload_send_json(req, "500 Internal Server Error",
                                "{\"error\":\"upload_failed\",\"message\":\"Upload failed\"}", content_len,
                                "handler_ctx_internal", ESP_FAIL);
    }

    if (ret == ESP_OK)
    {
        return upload_send_json(req, "200 OK", "{\"status\":\"ok\"}", content_len, "handler_success", ESP_OK);
    }

    /* Parser/callback failures: map to locked contract where applicable. */
    if (ret == ESP_ERR_INVALID_ARG)
    {
        return upload_send_json(
            req, "400 Bad Request",
            "{\"error\":\"upload_invalid_request\",\"message\":\"Upload request is malformed or invalid\"}",
            content_len, "handler_parse_invalid_request", ret);
    }

    if (ret == ESP_ERR_INVALID_STATE)
    {
        /* Incomplete/malformed multipart structures and callback-driven rejects. */
        return upload_send_json(
            req, "400 Bad Request",
            "{\"error\":\"upload_invalid_request\",\"message\":\"Upload request is malformed or invalid\"}",
            content_len, "handler_parse_invalid_request_state", ret);
    }

    if (ret == ESP_ERR_NO_MEM)
    {
        /* Runtime allocation/accumulator exhaustion: fail safely (no partial write). */
        return upload_send_json(req, "503 Service Unavailable",
                                "{\"error\":\"upload_rejected_low_memory\","
                                "\"message\":\"Upload rejected: insufficient contiguous memory\"}",
                                content_len, "handler_parse_low_memory", ret);
    }

    /* recv/socket/other failures: best-effort abort classification and cleanup already done. */
    ESP_LOGW(TAG, "Upload parse/transfer failed: %s", esp_err_to_name(ret));
    return upload_send_json(req, "503 Service Unavailable",
                            "{\"error\":\"upload_aborted\",\"message\":\"Upload aborted\"}", content_len,
                            "handler_aborted", ret);
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
