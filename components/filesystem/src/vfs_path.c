#include "filesystem.h"
#include "flash.h"
#include "sdcard.h"

#include <stdio.h>
#include <string.h>

#define VFS_COMPOSE_MAX 256

static esp_err_t normalize_absolute(const char *input, char *out, size_t len)
{
    if (input[0] != '/')
    {
        return ESP_ERR_INVALID_ARG;
    }

    size_t out_len = 0;
    const char *cursor = input;
    while (*cursor != '\0')
    {
        while (*cursor == '/')
        {
            ++cursor;
        }
        if (*cursor == '\0')
        {
            break;
        }

        const char *start = cursor;
        while (*cursor != '\0' && *cursor != '/')
        {
            ++cursor;
        }
        const size_t segment_len = (size_t)(cursor - start);
        if (segment_len == 1 && start[0] == '.')
        {
            continue;
        }
        if (segment_len == 2 && start[0] == '.' && start[1] == '.')
        {
            if (out_len == 0)
            {
                return ESP_ERR_INVALID_ARG;
            }
            while (out_len > 0 && out[out_len - 1] != '/')
            {
                --out_len;
            }
            if (out_len > 0)
            {
                --out_len;
            }
            continue;
        }

        if (out_len + 1 + segment_len >= len)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        out[out_len++] = '/';
        memcpy(out + out_len, start, segment_len);
        out_len += segment_len;
    }

    if (out_len == 0)
    {
        if (len < 2)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        out[0] = '/';
        out[1] = '\0';
        return ESP_OK;
    }

    out[out_len] = '\0';
    return ESP_OK;
}

static esp_err_t join_script_path(const char *script_file, const char *input, char *combined, size_t combined_len)
{
    if (input[0] == '/')
    {
        const size_t length = strlen(input);
        if (length >= combined_len)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        memcpy(combined, input, length + 1);
        return ESP_OK;
    }

    if (!(input[0] == '.' && input[1] == '/'))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (script_file == NULL || script_file[0] != '/')
    {
        return ESP_ERR_INVALID_STATE;
    }

    const char *slash = strrchr(script_file, '/');
    if (slash == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    const size_t dir_len = (size_t)(slash - script_file);
    const char *rest = input + 1;
    const size_t rest_len = strlen(rest);
    if (dir_len >= combined_len || rest_len >= combined_len - dir_len)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(combined, script_file, dir_len);
    memcpy(combined + dir_len, rest, rest_len + 1);
    return ESP_OK;
}

esp_err_t vfs_compose_script_path(const char *script_file, const char *input, char *resolved, size_t len)
{
    if (input == NULL || resolved == NULL || len == 0 || input[0] == '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }

    char combined[VFS_COMPOSE_MAX];
    const esp_err_t joined = join_script_path(script_file, input, combined, sizeof(combined));
    if (joined != ESP_OK)
    {
        return joined;
    }
    return normalize_absolute(combined, resolved, len);
}

bool vfs_is_virtual_path(const char *path)
{
    if (path == NULL || path[0] != '/')
    {
        return false;
    }
    if (strcmp(path, "/") == 0)
    {
        return true;
    }
    if (strcmp(path, "/flash") == 0 || strncmp(path, "/flash/", 7) == 0)
    {
        return true;
    }
    if (strcmp(path, "/sdcard") == 0 || strncmp(path, "/sdcard/", 8) == 0)
    {
        return true;
    }
    return false;
}

esp_err_t vfs_resolve_path(const char *virtual_path, char *real_path, size_t len)
{
    if (virtual_path == NULL || real_path == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char normalized[VFS_COMPOSE_MAX] = {0};
    esp_err_t err = normalize_absolute(virtual_path, normalized, sizeof(normalized));
    if (err != ESP_OK)
    {
        return err;
    }

    if (strncmp(normalized, "/flash", 6) == 0)
    {
        const char *rest = normalized + 6;
        if (rest[0] == '\0')
        {
            rest = "/";
        }
        int written = snprintf(real_path, len, "%s%s", FLASH_MOUNT_POINT, rest);
        if (written < 0 || (size_t)written >= len)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        return ESP_OK;
    }

    if (strncmp(normalized, "/sdcard", 7) == 0)
    {
        const char *rest = normalized + 7;
        if (rest[0] == '\0')
        {
            rest = "/";
        }
        int written = snprintf(real_path, len, "%s%s", SDCARD_MOUNT_POINT, rest);
        if (written < 0 || (size_t)written >= len)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}
