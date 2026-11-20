#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Return a canonical key name for an SS3 final byte (ESC O <final>), or NULL
     * when the sequence is not recognized.
     */
    const char *shell_keyparse_ss3_name(char final_byte);

    /**
     * Return a canonical key name for a CSI sequence (ESC [ [params] <final>), or
     * NULL when the sequence is not recognized. param is the first numeric
     * parameter (0 when absent).
     */
    const char *shell_keyparse_csi_name(int param, char final_byte);

#ifdef __cplusplus
}
#endif
