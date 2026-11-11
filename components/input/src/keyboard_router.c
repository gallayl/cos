#include "keyboard_router.h"

void keyboard_route_event(const keyboard_event_t *event, const keyboard_router_t *router)
{
    if (event == NULL || router == NULL)
    {
        return;
    }

    if (router->console_owns_input != NULL && router->console_owns_input(router->context)) // NOLINT(readability-implicit-bool-conversion) - callback bool result in C condition
    {
        if (event->pressed && event->text_len > 0 && router->shell_input != NULL) // NOLINT(readability-implicit-bool-conversion) - bool/size checks in C condition
        {
            router->shell_input(event->text, event->text_len, router->context);
        }
        return;
    }

    if (router->gui_handles_event != NULL && router->gui_handles_event(event, router->context)) // NOLINT(readability-implicit-bool-conversion) - callback bool result in C condition
    {
        return;
    }

    if (event->pressed && event->text_len > 0 && router->shell_input != NULL) // NOLINT(readability-implicit-bool-conversion) - bool/size checks in C condition
    {
        router->shell_input(event->text, event->text_len, router->context);
    }
}
