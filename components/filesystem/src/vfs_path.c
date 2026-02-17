#include "filesystem.h"
#include "flash.h"
#include "sdcard.h"

#include <stdio.h>
#include <string.h>

esp_err_t vfs_resolve_path(const char *virtual_path, char *real_path, size_t len)
{
    if (virtual_path == NULL || real_path == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (strncmp(virtual_path, "/flash", 6) == 0)
    {
        const char *rest = virtual_path + 6;
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

    if (strncmp(virtual_path, "/sdcard", 7) == 0)
    {
        const char *rest = virtual_path + 7;
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
