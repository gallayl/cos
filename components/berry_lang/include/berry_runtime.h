#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** Create the Berry virtual machine. Repeated calls are safe. */
    esp_err_t berry_runtime_init(void);

    /** Destroy the Berry virtual machine if it has been initialized. */
    void berry_runtime_deinit(void);

    /** Compile and execute Berry source code in the shared VM. */
    esp_err_t berry_runtime_eval(const char *source, const char *source_name);

    /** Load and execute a Berry script from a COS virtual filesystem path. */
    esp_err_t berry_runtime_run_file(const char *virtual_path);

    /** Load a class-based Berry UI app and invoke its setup method. */
    esp_err_t berry_runtime_open_app(const char *virtual_path);

    /** Invoke the current app's teardown method and release its UI resources. */
    esp_err_t berry_runtime_close_app(void);

#ifdef __cplusplus
}
#endif
