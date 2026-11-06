#include "network.h"

#include <stdio.h>
#include <string.h>

static char s_base[NETWORK_URL_MAX + 1];

static bool is_digit(unsigned char character)
{
    return character >= '0' && character <= '9';
}

static bool is_alpha(unsigned char character)
{
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}

static bool is_alnum(unsigned char character)
{
    return is_alpha(character) || is_digit(character);
}

static bool is_url_char(unsigned char character)
{
    if (is_alnum(character))
    {
        return true;
    }
    switch (character)
    {
    case '-':
    case '.':
    case '_':
    case '~':
    case ':':
    case '/':
    case '?':
    case '&':
    case '=':
    case '%':
    case '+':
    case '@':
    case '!':
    case '$':
    case '\'':
    case '(':
    case ')':
    case '*':
    case ',':
    case ';':
        return true;
    default:
        return false;
    }
}

static bool is_header_token(unsigned char character)
{
    if (is_alnum(character))
    {
        return true;
    }
    switch (character)
    {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
        return true;
    default:
        return false;
    }
}

static bool equals_ignore_case(const char *left, const char *right, size_t length)
{
    for (size_t index = 0; index < length; ++index)
    {
        unsigned char lhs = (unsigned char)left[index];
        unsigned char rhs = (unsigned char)right[index];
        if (lhs >= 'A' && lhs <= 'Z')
        {
            lhs = (unsigned char)(lhs - 'A' + 'a');
        }
        if (rhs >= 'A' && rhs <= 'Z')
        {
            rhs = (unsigned char)(rhs - 'A' + 'a');
        }
        if (lhs != rhs)
        {
            return false;
        }
    }
    return true;
}

static bool valid_hostname(const char *host)
{
    if (host == NULL || host[0] == '\0' || strlen(host) > NETWORK_HOST_MAX)
    {
        return false;
    }

    size_t label_length = 0;
    bool previous_hyphen = false;
    for (const unsigned char *character = (const unsigned char *)host; *character != '\0'; ++character)
    {
        if (*character == '.')
        {
            if (label_length == 0 || previous_hyphen)
            {
                return false;
            }
            label_length = 0;
            previous_hyphen = false;
            continue;
        }
        if (!is_alnum(*character) && *character != '-')
        {
            return false;
        }
        if (label_length == 0 && *character == '-')
        {
            return false;
        }
        previous_hyphen = *character == '-';
        if (++label_length > 63)
        {
            return false;
        }
    }
    return label_length > 0 && !previous_hyphen;
}

static bool chars_allowed(const char *url)
{
    for (const unsigned char *character = (const unsigned char *)url; *character != '\0'; ++character)
    {
        if (!is_url_char(*character))
        {
            return false;
        }
    }
    return true;
}

static bool scheme_rest(const char *url, const char *scheme, size_t scheme_length, const char **rest)
{
    if (!equals_ignore_case(url, scheme, scheme_length) || url[scheme_length] != ':')
    {
        return false;
    }
    if (url[scheme_length + 1] != '/' || url[scheme_length + 2] != '/')
    {
        return false;
    }
    *rest = url + scheme_length + 3;
    return true;
}

static bool http_scheme(const char *url, bool *https, const char **rest)
{
    if (scheme_rest(url, "https", 5, rest))
    {
        *https = true;
        return true;
    }
    if (scheme_rest(url, "http", 4, rest))
    {
        *https = false;
        return true;
    }
    return false;
}

static bool looks_like_host_url(const char *url)
{
    if (url[0] == '/' || url[0] == '?' || url[0] == '.')
    {
        return false;
    }

    const char *end = url;
    while (*end != '\0' && *end != '/' && *end != '?' && *end != ':')
    {
        ++end;
    }
    const size_t host_length = (size_t)(end - url);
    if (host_length == 0 || host_length > NETWORK_HOST_MAX)
    {
        return false;
    }

    bool dot = false;
    char host[NETWORK_HOST_MAX + 1];
    for (size_t index = 0; index < host_length; ++index)
    {
        host[index] = url[index];
        if (url[index] == '.')
        {
            dot = true;
        }
    }
    host[host_length] = '\0';
    return dot && valid_hostname(host);
}

typedef struct
{
    bool https;
    char host[NETWORK_HOST_MAX + 1];
    uint16_t port;
    char path[NETWORK_URL_MAX + 1];
    char query[NETWORK_URL_MAX + 1];
} url_parts_t;

static bool parse_port(const char *start, const char **end, uint16_t *port)
{
    if (!is_digit((unsigned char)*start))
    {
        return false;
    }

    unsigned value = 0;
    const char *cursor = start;
    while (is_digit((unsigned char)*cursor))
    {
        value = value * 10u + (unsigned)(*cursor - '0');
        if (value > 65535u)
        {
            return false;
        }
        ++cursor;
    }
    if (cursor == start || value == 0)
    {
        return false;
    }
    *port = (uint16_t)value;
    *end = cursor;
    return true;
}

static esp_err_t copy_path_and_query(const char *start, url_parts_t *parts)
{
    if (*start == '\0')
    {
        memcpy(parts->path, "/", 2);
        parts->query[0] = '\0';
        return ESP_OK;
    }
    if (*start == '?')
    {
        if (strlen(start + 1) > NETWORK_URL_MAX)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        memcpy(parts->path, "/", 2);
        memcpy(parts->query, start + 1, strlen(start + 1) + 1);
        return ESP_OK;
    }
    if (*start != '/')
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *query = strchr(start, '?');
    const size_t path_length = query != NULL ? (size_t)(query - start) : strlen(start);
    if (path_length == 0 || path_length > NETWORK_URL_MAX)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(parts->path, start, path_length);
    parts->path[path_length] = '\0';
    if (query == NULL)
    {
        parts->query[0] = '\0';
        return ESP_OK;
    }
    if (strlen(query + 1) > NETWORK_URL_MAX)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(parts->query, query + 1, strlen(query + 1) + 1);
    return ESP_OK;
}

static esp_err_t parse_absolute(const char *url, url_parts_t *parts)
{
    memset(parts, 0, sizeof(*parts));
    const char *authority = NULL;
    if (!http_scheme(url, &parts->https, &authority))
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *host_end = authority;
    while (*host_end != '\0' && *host_end != '/' && *host_end != '?' && *host_end != ':')
    {
        if (*host_end == '@')
        {
            return ESP_ERR_INVALID_ARG;
        }
        ++host_end;
    }
    const size_t host_length = (size_t)(host_end - authority);
    if (host_length == 0 || host_length > NETWORK_HOST_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t index = 0; index < host_length; ++index)
    {
        char character = authority[index];
        if (character >= 'A' && character <= 'Z')
        {
            character = (char)(character - 'A' + 'a');
        }
        parts->host[index] = character;
    }
    parts->host[host_length] = '\0';
    if (!valid_hostname(parts->host))
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *path = host_end;
    if (*host_end == ':')
    {
        if (!parse_port(host_end + 1, &path, &parts->port))
        {
            return ESP_ERR_INVALID_ARG;
        }
        if (*path != '\0' && *path != '/' && *path != '?')
        {
            return ESP_ERR_INVALID_ARG;
        }
    }
    if ((!parts->https && parts->port == 80) || (parts->https && parts->port == 443))
    {
        parts->port = 0;
    }
    return copy_path_and_query(path, parts);
}

static esp_err_t format_url(const url_parts_t *parts, char *out, size_t out_size)
{
    const char *scheme = parts->https ? "https" : "http";
    int written;
    if (parts->port == 0)
    {
        written = parts->query[0] == '\0'
                      ? snprintf(out, out_size, "%s://%s%s", scheme, parts->host, parts->path)
                      : snprintf(out, out_size, "%s://%s%s?%s", scheme, parts->host, parts->path, parts->query);
    }
    else
    {
        written = parts->query[0] == '\0'
                      ? snprintf(out, out_size, "%s://%s:%u%s", scheme, parts->host, (unsigned)parts->port, parts->path)
                      : snprintf(out, out_size, "%s://%s:%u%s?%s", scheme, parts->host, (unsigned)parts->port,
                                 parts->path, parts->query);
    }
    if (written < 0 || (size_t)written >= out_size)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

static esp_err_t remove_dot_segments(const char *path, char *out, size_t out_size)
{
    if (path == NULL || path[0] != '/')
    {
        return ESP_ERR_INVALID_ARG;
    }

    const char *starts[48];
    size_t lengths[48];
    size_t count = 0;
    const char *cursor = path;
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
        const char *begin = cursor;
        while (*cursor != '\0' && *cursor != '/')
        {
            ++cursor;
        }
        const size_t length = (size_t)(cursor - begin);
        if (length == 1 && begin[0] == '.')
        {
            continue;
        }
        if (length == 2 && begin[0] == '.' && begin[1] == '.')
        {
            if (count > 0)
            {
                --count;
            }
            continue;
        }
        if (count >= 48)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        starts[count] = begin;
        lengths[count] = length;
        ++count;
    }

    if (count == 0)
    {
        if (out_size < 2)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        memcpy(out, "/", 2);
        return ESP_OK;
    }

    size_t used = 0;
    for (size_t index = 0; index < count; ++index)
    {
        if (used + 1 + lengths[index] >= out_size)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        out[used++] = '/';
        memcpy(out + used, starts[index], lengths[index]);
        used += lengths[index];
    }
    if (path[strlen(path) - 1] == '/')
    {
        if (used + 1 >= out_size)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        out[used++] = '/';
    }
    out[used] = '\0';
    return ESP_OK;
}

static esp_err_t normalize_absolute(const char *url, char *out, size_t out_size)
{
    url_parts_t parts;
    esp_err_t err = parse_absolute(url, &parts);
    if (err != ESP_OK)
    {
        return err;
    }
    char path[NETWORK_URL_MAX + 1];
    err = remove_dot_segments(parts.path, path, sizeof(path));
    if (err != ESP_OK)
    {
        return err;
    }
    memcpy(parts.path, path, strlen(path) + 1);
    return format_url(&parts, out, out_size);
}

static bool url_is_absolute(const char *url)
{
    bool https = false;
    const char *rest = NULL;
    return http_scheme(url, &https, &rest) || looks_like_host_url(url);
}

static esp_err_t join_relative(const char *base, const char *url, char *out, size_t out_size)
{
    url_parts_t parts;
    esp_err_t err = parse_absolute(base, &parts);
    if (err != ESP_OK)
    {
        return err;
    }

    const char *query = strchr(url, '?');
    char reference[NETWORK_URL_MAX + 1];
    if (query == NULL)
    {
        if (strlen(url) > NETWORK_URL_MAX)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        memcpy(reference, url, strlen(url) + 1);
    }
    else
    {
        const size_t path_length = (size_t)(query - url);
        if (path_length > NETWORK_URL_MAX || strlen(query + 1) > NETWORK_URL_MAX)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        memcpy(reference, url, path_length);
        reference[path_length] = '\0';
        memcpy(parts.query, query + 1, strlen(query + 1) + 1);
    }

    char merged[NETWORK_URL_MAX + 1];
    if (reference[0] == '\0')
    {
        /* Query-only reference. The base path stays. */
    }
    else if (reference[0] == '/')
    {
        err = remove_dot_segments(reference, merged, sizeof(merged));
        if (err != ESP_OK)
        {
            return err;
        }
        memcpy(parts.path, merged, strlen(merged) + 1);
        if (query == NULL)
        {
            parts.query[0] = '\0';
        }
    }
    else
    {
        const char *slash = strrchr(parts.path, '/');
        if (slash == NULL)
        {
            return ESP_ERR_INVALID_ARG;
        }
        const size_t prefix_length = (size_t)(slash - parts.path) + 1;
        if (prefix_length + strlen(reference) > NETWORK_URL_MAX)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        memcpy(merged, parts.path, prefix_length);
        memcpy(merged + prefix_length, reference, strlen(reference) + 1);
        err = remove_dot_segments(merged, parts.path, sizeof(parts.path));
        if (err != ESP_OK)
        {
            return err;
        }
    }
    return format_url(&parts, out, out_size);
}

bool network_http_method_valid(const char *method)
{
    return method != NULL &&
           (strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0 ||
            strcmp(method, "PATCH") == 0 || strcmp(method, "DELETE") == 0 || strcmp(method, "HEAD") == 0);
}

bool network_http_method_allows_body(const char *method)
{
    return method != NULL && strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0 &&
           network_http_method_valid(method);
}

bool network_http_header_valid(const char *name, const char *value)
{
    if (name == NULL || value == NULL || name[0] == '\0')
    {
        return false;
    }
    const size_t name_length = strlen(name);
    const size_t value_length = strlen(value);
    if (name_length > NETWORK_HTTP_HEADER_NAME_MAX || value_length > NETWORK_HTTP_HEADER_VALUE_MAX)
    {
        return false;
    }
    for (size_t index = 0; index < name_length; ++index)
    {
        if (!is_header_token((unsigned char)name[index]))
        {
            return false;
        }
    }
    for (size_t index = 0; index < value_length; ++index)
    {
        const unsigned char character = (unsigned char)value[index];
        if (character < 0x20 || character > 0x7e)
        {
            return false;
        }
    }
    return true;
}

bool network_http_timeout_valid(uint32_t timeout_ms)
{
    return timeout_ms >= NETWORK_HTTP_TIMEOUT_MIN_MS && timeout_ms <= NETWORK_HTTP_TIMEOUT_MAX_MS;
}

esp_err_t network_compose_url(const char *url, char *out, size_t out_size)
{
    if (url == NULL || url[0] == '\0' || out == NULL || out_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(url) > NETWORK_URL_MAX || !chars_allowed(url))
    {
        return strlen(url) > NETWORK_URL_MAX ? ESP_ERR_INVALID_SIZE : ESP_ERR_INVALID_ARG;
    }

    if (url_is_absolute(url))
    {
        char absolute[NETWORK_URL_MAX + 1];
        const char *source = url;
        bool https = false;
        const char *rest = NULL;
        if (!http_scheme(url, &https, &rest))
        {
            const int written = snprintf(absolute, sizeof(absolute), "http://%s", url);
            if (written < 0 || (size_t)written >= sizeof(absolute))
            {
                return ESP_ERR_INVALID_SIZE;
            }
            source = absolute;
        }
        return normalize_absolute(source, out, out_size);
    }

    if (strstr(url, "://") != NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_base[0] == '\0')
    {
        return ESP_ERR_INVALID_STATE;
    }
    return join_relative(s_base, url, out, out_size);
}

esp_err_t network_set_base_url(const char *url)
{
    if (url == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (url[0] == '\0')
    {
        s_base[0] = '\0';
        return ESP_OK;
    }
    if (!url_is_absolute(url))
    {
        return ESP_ERR_INVALID_ARG;
    }

    char normalized[NETWORK_URL_MAX + 1];
    const esp_err_t err = network_compose_url(url, normalized, sizeof(normalized));
    if (err != ESP_OK)
    {
        return err;
    }
    if (strchr(normalized, '?') != NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(s_base, normalized, strlen(normalized) + 1);
    return ESP_OK;
}

esp_err_t network_get_base_url(char *out, size_t out_size)
{
    if (out == NULL || out_size == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(s_base) >= out_size)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(out, s_base, strlen(s_base) + 1);
    return ESP_OK;
}
