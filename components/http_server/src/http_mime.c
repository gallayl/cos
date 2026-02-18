#include "http_mime.h"

#include <stdbool.h>
#include <string.h>

static bool ends_with(const char *str, const char *suffix)
{
    size_t str_len = strlen(str);
    size_t suf_len = strlen(suffix);
    if (suf_len > str_len)
    {
        return false;
    }
    return strcmp(str + str_len - suf_len, suffix) == 0;
}

const char *http_mime_type(const char *path)
{
    if (path == NULL || *path == '\0')
    {
        return "application/octet-stream";
    }

    if (ends_with(path, ".html") || ends_with(path, ".htm"))
    {
        return "text/html";
    }
    if (ends_with(path, ".js"))
    {
        return "application/javascript";
    }
    if (ends_with(path, ".css"))
    {
        return "text/css";
    }
    if (ends_with(path, ".json"))
    {
        return "application/json";
    }
    if (ends_with(path, ".png"))
    {
        return "image/png";
    }
    if (ends_with(path, ".jpg") || ends_with(path, ".jpeg"))
    {
        return "image/jpeg";
    }
    if (ends_with(path, ".gif"))
    {
        return "image/gif";
    }
    if (ends_with(path, ".ico"))
    {
        return "image/x-icon";
    }
    if (ends_with(path, ".svg"))
    {
        return "image/svg+xml";
    }
    if (ends_with(path, ".woff"))
    {
        return "font/woff";
    }
    if (ends_with(path, ".woff2"))
    {
        return "font/woff2";
    }
    return "application/octet-stream";
}
