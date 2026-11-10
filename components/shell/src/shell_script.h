#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Register the `path` command and load its persisted configuration. */
    esp_err_t shell_script_init(void);

    /**
     * Resolve and run a Berry command script for tokenized arguments.
     * `argv[0]` is the command token (after alias expansion). `explicit_path`
     * is true when that token contains a virtual path separator.
     *
     * On ESP_OK with *handled=true, a script ran and *ret is its shell status.
     * On ESP_OK with *handled=false, no script applied; caller may report
     * unknown command for non-explicit input.
     * Other errors with *handled=true indicate a script/path error that was
     * reported; caller should treat as command failure without printing
     * "Unknown command".
     */
    esp_err_t shell_script_try_exec(int argc, char **argv, bool explicit_path, int *ret, bool *handled);

    /**
     * Dispatch one expanded command line through native commands, then script
     * resolution.
     *
     * Returns ESP_OK when the line was handled: a native command ran, or a
     * Berry command script ran/reported an error. *ret receives the command or
     * script status (1 for resolver-reported script/path errors).
     * Returns ESP_ERR_NOT_FOUND when no native command or script matched.
     * Other errors are dispatch infrastructure failures (for example memory
     * allocation) or esp_console errors other than ESP_ERR_NOT_FOUND.
     */
    esp_err_t shell_dispatch_expanded_line(const char *line, int *ret);

#ifdef __cplusplus
}
#endif
