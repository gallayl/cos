#include "berry.h"
#include "light_sensor.h"

#include "esp_err.h"

namespace
{
void map_set_bool(bvm *vm, const char *key, bool value)
{
    be_pushstring(vm, key);
    be_pushbool(vm, value ? 1 : 0);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

void map_set_string(bvm *vm, const char *key, const char *value)
{
    be_pushstring(vm, key);
    be_pushstring(vm, value != nullptr ? value : "");
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

void map_set_int(bvm *vm, const char *key, bint value)
{
    be_pushstring(vm, key);
    be_pushint(vm, value);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

int native_light_read(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        be_raise(vm, "value_error", "light.read() takes no arguments");
        be_return(vm);
    }

    const int value = light_sensor_read();
    be_newobject(vm, "map");
    if (value < 0)
    {
        map_set_bool(vm, "ok", false);
        map_set_string(vm, "error", esp_err_to_name(ESP_FAIL));
    }
    else
    {
        map_set_bool(vm, "ok", true);
        map_set_int(vm, "value", static_cast<bint>(value));
    }
    be_pop(vm, 1);
    be_return(vm);
}
} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
static bntvmodobj_t const light_attrs[] = {
    be_native_module_function("read", native_light_read),
};

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" const bntvmodule_t be_native_module_light = {
    "light",
    light_attrs,
    sizeof(light_attrs) / sizeof(light_attrs[0]),
    nullptr,
};
