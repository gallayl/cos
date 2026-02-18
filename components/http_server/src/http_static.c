#include "http_mime.h"
#include "http_server.h"

#include "esp_log.h"
#include "filesystem.h"
#include "nvs.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char *const TAG = "http_static";

#define NVS_NAMESPACE "http_srv"
#define NVS_KEY_ENABLED "static_on"
#define NVS_KEY_ROOT "static_root"
#define NVS_KEY_SPA "spa"

#define STATIC_ROOT_MAX 64
#define DEFAULT_ROOT "/flash/public"
#define CHUNK_SIZE 1024
#define LITTLEFS_BASE "/littlefs"
#define REAL_PATH_MAX 192

static bool s_enabled = false;
static bool s_spa = false;
static char s_root[STATIC_ROOT_MAX] = DEFAULT_ROOT;

static void load_config(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK)
    {
        return;
    }

    uint8_t val = 0;
    size_t len = sizeof(val);
    if (nvs_get_blob(h, NVS_KEY_ENABLED, &val, &len) == ESP_OK)
    {
        s_enabled = (val != 0);
    }

    len = sizeof(s_root);
    if (nvs_get_blob(h, NVS_KEY_ROOT, s_root, &len) != ESP_OK)
    {
        strncpy(s_root, DEFAULT_ROOT, sizeof(s_root) - 1);
    }
    s_root[sizeof(s_root) - 1] = '\0';

    val = 0;
    len = sizeof(val);
    if (nvs_get_blob(h, NVS_KEY_SPA, &val, &len) == ESP_OK)
    {
        s_spa = (val != 0);
    }

    nvs_close(h);
}

static esp_err_t save_uint8(const char *key, uint8_t val)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK)
    {
        return err;
    }
    nvs_set_blob(h, key, &val, sizeof(val));
    nvs_commit(h);
    nvs_close(h);
    return ESP_OK;
}

void http_static_init(void)
{
    load_config();
    ESP_LOGI(TAG, "Static: %s, root=%s, spa=%s", s_enabled ? "on" : "off", s_root, s_spa ? "on" : "off");
}

esp_err_t http_static_set_enabled(bool enabled)
{
    s_enabled = enabled;
    return save_uint8(NVS_KEY_ENABLED, enabled ? 1 : 0);
}

bool http_static_is_enabled(void)
{
    return s_enabled;
}

esp_err_t http_static_set_root(const char *virtual_path)
{
    if (virtual_path == NULL || strlen(virtual_path) >= STATIC_ROOT_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }
    strncpy(s_root, virtual_path, sizeof(s_root) - 1);
    s_root[sizeof(s_root) - 1] = '\0';

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK)
    {
        return err;
    }
    nvs_set_blob(h, NVS_KEY_ROOT, s_root, strlen(s_root) + 1);
    nvs_commit(h);
    nvs_close(h);
    return ESP_OK;
}

const char *http_static_get_root(void)
{
    return s_root;
}

esp_err_t http_static_set_spa(bool enabled)
{
    s_spa = enabled;
    return save_uint8(NVS_KEY_SPA, enabled ? 1 : 0);
}

bool http_static_is_spa(void)
{
    return s_spa;
}

static bool is_api_route(const char *uri)
{
    return (strncmp(uri, "/api/", 5) == 0 || strncmp(uri, "/update", 7) == 0 || strncmp(uri, "/ws", 3) == 0);
}

static esp_err_t serve_file(httpd_req_t *req, const char *real_path)
{
    FILE *f = fopen(real_path, "r");
    if (!f)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Read failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, http_mime_type(real_path));
    httpd_resp_set_hdr(req, "Cache-Control", "max-age=600");

    char chunk[CHUNK_SIZE];
    size_t n;
    do
    {
        n = fread(chunk, 1, sizeof(chunk), f);
        if (n > 0)
        {
            if (httpd_resp_send_chunk(req, chunk, n) != ESP_OK)
            {
                fclose(f);
                httpd_resp_send_chunk(req, NULL, 0);
                return ESP_FAIL;
            }
        }
    } while (n > 0);

    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t http_static_handler(httpd_req_t *req, httpd_err_code_t err_code)
{
    (void)err_code;

    if (!s_enabled)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_OK;
    }

    const char *uri = req->uri;

    if (is_api_route(uri))
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_OK;
    }

    /* Strip query string */
    char clean_uri[128];
    strncpy(clean_uri, uri, sizeof(clean_uri) - 1);
    clean_uri[sizeof(clean_uri) - 1] = '\0';
    char *q = strchr(clean_uri, '?');
    if (q)
    {
        *q = '\0';
    }

    if (strcmp(clean_uri, "/") == 0)
    {
        strncpy(clean_uri, "/index.html", sizeof(clean_uri) - 1);
    }

    /* Resolve root + URI to real filesystem path */
    char virtual_path[REAL_PATH_MAX];
    snprintf(virtual_path, sizeof(virtual_path), "%s%s", s_root, clean_uri);

    char real_path[REAL_PATH_MAX];
    if (vfs_resolve_path(virtual_path, real_path, sizeof(real_path)) != ESP_OK)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_OK;
    }

    struct stat st;
    if (stat(real_path, &st) == 0)
    {
        return serve_file(req, real_path);
    }

    /* SPA fallback: serve index.html for unmatched paths */
    if (s_spa)
    {
        char index_virtual[REAL_PATH_MAX];
        snprintf(index_virtual, sizeof(index_virtual), "%s/index.html", s_root);

        char index_real[REAL_PATH_MAX];
        if (vfs_resolve_path(index_virtual, index_real, sizeof(index_real)) == ESP_OK && stat(index_real, &st) == 0)
        {
            return serve_file(req, index_real);
        }
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
    return ESP_OK;
}
