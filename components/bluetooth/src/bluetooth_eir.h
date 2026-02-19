#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Parse a Bluetooth EIR (Extended Inquiry Response) record to extract
     * the device name (complete or shortened).
     *
     * @param eir       Pointer to the raw EIR data
     * @param eir_len   Length of the EIR data in bytes
     * @param out       Destination buffer for the null-terminated name
     * @param out_size  Size of the destination buffer
     * @return true if a name was found and written to @p out
     */
    bool bluetooth_parse_eir_name(const uint8_t *eir, size_t eir_len, char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
