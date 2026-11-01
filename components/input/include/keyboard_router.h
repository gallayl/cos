#pragma once

#include "keyboard_event.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Check whether a console prompt owns this keyboard event stream.
     * @param context Opaque router context.
     * @return True when the prompt owns keyboard input.
     */
    typedef bool (*keyboard_console_owner_fn)(void *context);
    /**
     * Offer an event to the active GUI app.
     * @param event Key transition to deliver.
     * @param context Opaque router context.
     * @return True when the GUI app consumed the event.
     */
    typedef bool (*keyboard_gui_owner_fn)(const keyboard_event_t *event, void *context);
    /**
     * Forward translated key-down text to the shell.
     * @param text Translated bytes.
     * @param length Number of valid bytes.
     * @param context Opaque router context.
     */
    typedef void (*keyboard_shell_input_fn)(const char *text, size_t length, void *context);

    /** Callback set used to route a key event to its current owner. */
    typedef struct
    {
        keyboard_console_owner_fn console_owns_input; /**< Prompt ownership check, or NULL. */
        keyboard_gui_owner_fn gui_handles_event;      /**< GUI event handler, or NULL. */
        keyboard_shell_input_fn shell_input;          /**< Shell text callback, or NULL. */
        void *context;                                /**< Opaque value passed to callbacks. */
    } keyboard_router_t;

    /**
     * Route an event to the prompt, then GUI app, then shell text input.
     * @param event Key transition to deliver.
     * @param router Owner callbacks and their context.
     */
    void keyboard_route_event(const keyboard_event_t *event, const keyboard_router_t *router);

#ifdef __cplusplus
}
#endif
