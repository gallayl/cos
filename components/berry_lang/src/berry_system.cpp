#include "berry.h"
#include "system.h"

#include <cstdint>

static int native_system_uptime(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        be_pushstring(vm, "");
        be_return(vm);
    }

    char buf[32];
    const esp_err_t err = system_format_uptime(buf, sizeof(buf));
    if (err != ESP_OK)
    {
        be_pushstring(vm, "");
        be_return(vm);
    }

    be_pushstring(vm, buf);
    be_return(vm);
}

// NOLINTNEXTLINE(readability-identifier-naming)
static bntvmodobj_t const system_attrs[] = {
    be_native_module_function("uptime", native_system_uptime),
};

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" const bntvmodule_t be_native_module_system = {
    "system",
    system_attrs,
    sizeof(system_attrs) / sizeof(system_attrs[0]),
    nullptr,
};
