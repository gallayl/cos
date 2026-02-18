#include "mbedtls/base64.h"

#include <string.h>

static const char B64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int b64_index(unsigned char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z')
    {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9')
    {
        return c - '0' + 52;
    }
    if (c == '+')
    {
        return 62;
    }
    if (c == '/')
    {
        return 63;
    }
    return -1;
}

int mbedtls_base64_decode(unsigned char *dst, size_t dlen, size_t *olen,
                          const unsigned char *src, size_t slen)
{
    size_t i = 0;
    size_t n = 0;

    /* Skip whitespace and padding for output length calc */
    size_t pad = 0;
    if (slen > 0 && src[slen - 1] == '=')
    {
        pad++;
    }
    if (slen > 1 && src[slen - 2] == '=')
    {
        pad++;
    }

    size_t out_len = (slen / 4) * 3 - pad;
    *olen = out_len;

    if (dst == NULL || dlen < out_len)
    {
        return -1;
    }

    while (i < slen)
    {
        int a = (i < slen) ? b64_index(src[i++]) : 0;
        int b = (i < slen) ? b64_index(src[i++]) : 0;
        int c = (i < slen) ? b64_index(src[i++]) : 0;
        int d = (i < slen) ? b64_index(src[i++]) : 0;

        if (a < 0 || b < 0)
        {
            return -1;
        }

        uint32_t triple = ((uint32_t)a << 18) | ((uint32_t)b << 12) | ((uint32_t)c << 6) | (uint32_t)d;

        if (n < out_len)
        {
            dst[n++] = (triple >> 16) & 0xFF;
        }
        if (n < out_len)
        {
            dst[n++] = (triple >> 8) & 0xFF;
        }
        if (n < out_len)
        {
            dst[n++] = triple & 0xFF;
        }
    }

    return 0;
}

int mbedtls_base64_encode(unsigned char *dst, size_t dlen, size_t *olen,
                          const unsigned char *src, size_t slen)
{
    size_t out_len = ((slen + 2) / 3) * 4;
    *olen = out_len;

    if (dst == NULL || dlen < out_len + 1)
    {
        return -1;
    }

    size_t i = 0;
    size_t j = 0;
    while (i < slen)
    {
        uint32_t a = (i < slen) ? src[i++] : 0;
        uint32_t b = (i < slen) ? src[i++] : 0;
        uint32_t c = (i < slen) ? src[i++] : 0;
        uint32_t triple = (a << 16) | (b << 8) | c;

        dst[j++] = (unsigned char)B64_CHARS[(triple >> 18) & 0x3F];
        dst[j++] = (unsigned char)B64_CHARS[(triple >> 12) & 0x3F];
        dst[j++] = (unsigned char)B64_CHARS[(triple >> 6) & 0x3F];
        dst[j++] = (unsigned char)B64_CHARS[triple & 0x3F];
    }

    /* Add padding */
    size_t mod = slen % 3;
    if (mod == 1)
    {
        dst[j - 1] = '=';
        dst[j - 2] = '=';
    }
    else if (mod == 2)
    {
        dst[j - 1] = '=';
    }
    dst[j] = '\0';

    return 0;
}
