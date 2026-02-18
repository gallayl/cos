#include "http_server.h"

#include <stdbool.h>
#include <string.h>

esp_err_t http_sanitize_upload_path(const char *raw, char *out_path, size_t len)
{
    if (raw == NULL || out_path == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (raw[0] == '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (strstr(raw, "..") != NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Normalize: copy while collapsing double slashes */
    size_t out_i = 0;
    bool prev_slash = false;
    for (size_t i = 0; raw[i] != '\0' && out_i < len - 1; i++)
    {
        char c = raw[i];
        if (c == '\\')
        {
            c = '/';
        }
        if (c == '/' && prev_slash)
        {
            continue;
        }
        prev_slash = (c == '/');
        out_path[out_i++] = c;
    }

    /* Remove trailing slash (unless root) */
    if (out_i > 1 && out_path[out_i - 1] == '/')
    {
        out_i--;
    }
    out_path[out_i] = '\0';

    /* Require /flash or /sdcard prefix */
    if (strncmp(out_path, "/flash", 6) != 0 && strncmp(out_path, "/sdcard", 7) != 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}
