#include "http_multipart.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define READ_BUF_SIZE 2048

static char *extract_boundary(httpd_req_t *req, char *buf, size_t buf_len)
{
    if (httpd_req_get_hdr_value_str(req, "Content-Type", buf, buf_len) != ESP_OK)
    {
        return NULL;
    }
    char *b = strstr(buf, "boundary=");
    if (!b)
    {
        return NULL;
    }
    b += 9;
    if (*b == '"')
    {
        b++;
        char *end = strchr(b, '"');
        if (end)
        {
            *end = '\0';
        }
    }
    else
    {
        char *end = b;
        while (*end && *end != ';' && *end != ' ' && *end != '\r' && *end != '\n')
        {
            end++;
        }
        *end = '\0';
    }
    return b;
}

static void parse_part_headers(const char *headers, size_t hdr_len, char *name, size_t name_len, char *filename,
                               size_t filename_len)
{
    name[0] = '\0';
    filename[0] = '\0';

    const char *cd = strstr(headers, "name=\"");
    if (!cd)
    {
        /* case-insensitive fallback */
        cd = strstr(headers, "Name=\"");
    }
    if (cd)
    {
        cd += 6;
        const char *end = strchr(cd, '"');
        if (end)
        {
            size_t n = (size_t)(end - cd);
            if (n >= name_len)
            {
                n = name_len - 1;
            }
            memcpy(name, cd, n);
            name[n] = '\0';
        }
    }

    const char *fn = strstr(headers, "filename=\"");
    if (fn)
    {
        fn += 10;
        const char *end = strchr(fn, '"');
        if (end)
        {
            size_t n = (size_t)(end - fn);
            if (n >= filename_len)
            {
                n = filename_len - 1;
            }
            memcpy(filename, fn, n);
            filename[n] = '\0';
        }
    }
}

esp_err_t http_multipart_parse(httpd_req_t *req, multipart_field_cb on_field, multipart_file_cb on_file, void *ctx)
{
    char ct_buf[256];
    char *boundary = extract_boundary(req, ct_buf, sizeof(ct_buf));
    if (!boundary)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char boundary_marker[128];
    snprintf(boundary_marker, sizeof(boundary_marker), "\r\n--%s", boundary);
    size_t bm_len = strlen(boundary_marker);

    char first_boundary[128];
    snprintf(first_boundary, sizeof(first_boundary), "--%s\r\n", boundary);
    size_t fb_len = strlen(first_boundary);

    char *buf = malloc(READ_BUF_SIZE + 1);
    if (!buf)
    {
        return ESP_ERR_NO_MEM;
    }

    char *accum = malloc(READ_BUF_SIZE * 4);
    if (!accum)
    {
        free(buf);
        return ESP_ERR_NO_MEM;
    }
    size_t accum_len = 0;

    enum
    {
        PREAMBLE,
        HEADER,
        BODY,
        AFTER_BOUNDARY
    } state = PREAMBLE;

    char field_name[64] = {0};
    char file_name[128] = {0};
    bool is_file = false;
    bool is_first_chunk = true;
    char field_value[512] = {0};
    size_t field_value_len = 0;

    int remaining = req->content_len;
    esp_err_t result = ESP_OK;

    while (1)
    {
        bool progress = true;
        while (progress)
        {
            progress = false;

            if (state == PREAMBLE)
            {
                char *pos = (accum_len >= fb_len) ? strstr(accum, first_boundary) : NULL;
                if (pos)
                {
                    size_t skip = (size_t)(pos - accum) + fb_len;
                    memmove(accum, accum + skip, accum_len - skip);
                    accum_len -= skip;
                    state = HEADER;
                    progress = true;
                }
            }

            if (state == HEADER)
            {
                char *pos = (accum_len >= 4) ? strstr(accum, "\r\n\r\n") : NULL;
                if (pos)
                {
                    *pos = '\0';
                    parse_part_headers(accum, (size_t)(pos - accum), field_name, sizeof(field_name), file_name,
                                       sizeof(file_name));
                    is_file = (file_name[0] != '\0');
                    is_first_chunk = true;
                    field_value_len = 0;

                    size_t skip = (size_t)(pos - accum) + 4;
                    memmove(accum, accum + skip, accum_len - skip);
                    accum_len -= skip;
                    state = BODY;
                    progress = true;
                }
            }

            if (state == BODY)
            {
                char *pos = (accum_len >= bm_len) ? strstr(accum, boundary_marker) : NULL;

                if (pos)
                {
                    size_t data_len = (size_t)(pos - accum);
                    if (is_file && on_file)
                    {
                        if (!on_file(field_name, file_name, (const uint8_t *)accum, data_len, is_first_chunk, true,
                                     ctx))
                        {
                            result = ESP_ERR_INVALID_STATE;
                            goto cleanup;
                        }
                    }
                    else if (!is_file && on_field)
                    {
                        size_t copy = data_len;
                        if (field_value_len + copy >= sizeof(field_value))
                        {
                            copy = sizeof(field_value) - field_value_len - 1;
                        }
                        memcpy(field_value + field_value_len, accum, copy);
                        field_value_len += copy;
                        field_value[field_value_len] = '\0';
                        on_field(field_name, field_value, ctx);
                    }

                    size_t skip = data_len + bm_len;
                    memmove(accum, accum + skip, accum_len - skip);
                    accum_len -= skip;
                    state = AFTER_BOUNDARY;
                    progress = true;
                }
                else if (accum_len > bm_len)
                {
                    size_t safe = accum_len - bm_len;
                    if (is_file && on_file)
                    {
                        if (!on_file(field_name, file_name, (const uint8_t *)accum, safe, is_first_chunk, false, ctx))
                        {
                            result = ESP_ERR_INVALID_STATE;
                            goto cleanup;
                        }
                        is_first_chunk = false;
                    }
                    else if (!is_file)
                    {
                        size_t copy = safe;
                        if (field_value_len + copy >= sizeof(field_value))
                        {
                            copy = sizeof(field_value) - field_value_len - 1;
                        }
                        memcpy(field_value + field_value_len, accum, copy);
                        field_value_len += copy;
                    }
                    memmove(accum, accum + safe, accum_len - safe);
                    accum_len -= safe;
                }
            }

            if (state == AFTER_BOUNDARY)
            {
                if (accum_len >= 2)
                {
                    if (accum[0] == '-' && accum[1] == '-')
                    {
                        goto cleanup;
                    }
                    /* skip \r\n after boundary */
                    memmove(accum, accum + 2, accum_len - 2);
                    accum_len -= 2;
                    state = HEADER;
                    progress = true;
                }
            }
        }

        if (remaining <= 0)
        {
            break;
        }

        int to_read = remaining < READ_BUF_SIZE ? remaining : READ_BUF_SIZE;
        int recvd = httpd_req_recv(req, buf, to_read);
        if (recvd <= 0)
        {
            if (recvd == HTTPD_SOCK_ERR_TIMEOUT)
            {
                continue;
            }
            break;
        }
        remaining -= recvd;

        memcpy(accum + accum_len, buf, (size_t)recvd);
        accum_len += (size_t)recvd;
        accum[accum_len] = '\0';
    }

    if (state == BODY && accum_len > 0)
    {
        if (is_file && on_file)
        {
            on_file(field_name, file_name, (const uint8_t *)accum, accum_len, is_first_chunk, true, ctx);
        }
        else if (!is_file && on_field)
        {
            size_t copy = accum_len;
            if (field_value_len + copy >= sizeof(field_value))
            {
                copy = sizeof(field_value) - field_value_len - 1;
            }
            memcpy(field_value + field_value_len, accum, copy);
            field_value_len += copy;
            field_value[field_value_len] = '\0';
            on_field(field_name, field_value, ctx);
        }
    }

cleanup:
    free(buf);
    free(accum);
    return result;
}
