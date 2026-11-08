#pragma once

#include "esp_err.h"
#include "keyboard_event.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*berry_console_line_callback_t)(esp_err_t result, const char *line, void *context);
    typedef esp_err_t (*berry_console_read_line_fn)(const char *prompt, berry_console_line_callback_t callback,
                                                    void *context);
    typedef esp_err_t (*berry_console_cancel_line_fn)(void *context);
    typedef esp_err_t (*berry_console_read_key_fn)(berry_console_line_callback_t callback, void *context);

    /** Create the Berry virtual machine. Repeated calls are safe. */
    esp_err_t berry_runtime_init(void);

    /** Destroy the Berry virtual machine if it has been initialized. */
    void berry_runtime_deinit(void);

    /** Compile and execute Berry source code in the shared VM. */
    esp_err_t berry_runtime_eval(const char *source, const char *source_name);

    /** Load and execute a Berry script from a COS virtual filesystem path. */
    esp_err_t berry_runtime_run_file(const char *virtual_path);

    /**
     * Execute a Berry command script without changing UI app state.
     * The script must define main(args), returning an integer shell status.
     */
    esp_err_t berry_runtime_exec_file(const char *virtual_path, int argc, char *const argv[], int *exit_status);

    /** Load a class-based Berry UI app and invoke its setup method. */
    esp_err_t berry_runtime_open_app(const char *virtual_path);

    /** Invoke the current app's teardown method and release its UI resources. */
    esp_err_t berry_runtime_close_app(void);

    /**
     * Deliver a HID key transition to the active Berry UI app.
     * Returns true when the GUI owns input; false leaves it for the shell.
     */
    bool berry_runtime_handle_key_event(const keyboard_event_t *event);

    /** Install console line, key, and character input without coupling Berry to the shell. */
    void berry_runtime_set_console_input_provider(berry_console_read_line_fn read_line,
                                                  berry_console_cancel_line_fn cancel_line,
                                                  berry_console_read_key_fn read_key,
                                                  berry_console_read_key_fn read_char);

    typedef struct
    {
        bool initialized;
        size_t gc_managed_bytes;
    } berry_runtime_memory_info_t;

    esp_err_t berry_runtime_get_memory_info(berry_runtime_memory_info_t *info);

    /**
     * Run Berry garbage collection and report managed bytes before/after.
     * Out parameters may be NULL. Returns ESP_ERR_INVALID_STATE if the VM is not initialized.
     */
    esp_err_t berry_runtime_gc_collect(size_t *before_bytes, size_t *after_bytes);

#ifdef __cplusplus
}
#endif

struct bvm;

/** Ensure the Berry `json` module is loaded. Safe to call repeatedly. */
#ifdef __cplusplus
extern "C"
#endif
    bool berry_runtime_ensure_json(struct bvm *vm);
