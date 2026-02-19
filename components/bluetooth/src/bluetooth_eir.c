#include "bluetooth_eir.h"

#include <string.h>

bool bluetooth_parse_eir_name(const uint8_t *eir, size_t eir_len, char *out, size_t out_size)
{
    if (eir == NULL || eir_len == 0)
    {
        return false;
    }

    size_t pos = 0;
    while (pos < eir_len)
    {
        uint8_t len = eir[pos];
        if (len == 0 || pos + 1 + len > eir_len)
        {
            break;
        }
        uint8_t type = eir[pos + 1];
        /* 0x09 = Complete Local Name, 0x08 = Shortened Local Name */
        if (type == 0x09 || type == 0x08)
        {
            size_t name_len = len - 1;
            if (name_len >= out_size)
            {
                name_len = out_size - 1;
            }
            memcpy(out, &eir[pos + 2], name_len);
            out[name_len] = '\0';
            return true;
        }
        pos += 1 + len;
    }
    return false;
}
