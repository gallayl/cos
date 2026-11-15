#include "system.h"

#include "berry.h"
#include "esp_err.h"

#include <cstdint>
#include <cstring>

namespace
{
static int native_system_meminfo(bvm *vm)
{
    system_info_t info;
    const esp_err_t err = system_get_info(&info);
    if (err != ESP_OK)
    {
        be_map_new(vm);
        be_pushbool(vm, bfalse);
        be_data_push(vm, (const char *)"ok");
        be_pop(vm, 1);
        be_pushstring(vm, esp_err_to_name(err));
        be_data_push(vm, (const char *)"error");
        be_pop(vm, 1);
        be_return(vm);
    }

    be_map_new(vm);
    be_pushbool(vm, btrue);
    be_data_push(vm, (const char *)"ok");
    be_pop(vm, 1);

    be_pushint(vm, (bint)info.free_heap);
    be_data_push(vm, (const char *)"free_heap");
    be_pop(vm, 1);
    be_pushint(vm, (bint)info.min_free_heap);
    be_data_push(vm, (const char *)"min_free_heap");
    be_pop(vm, 1);
    be_pushint(vm, (bint)info.total_heap);
    be_data_push(vm, (const char *)"total_heap");
    be_pop(vm, 1);
    be_pushint(vm, (bint)info.max_alloc_block);
    be_data_push(vm, (const char *)"max_alloc_block");
    be_pop(vm, 1);
    be_pushint(vm, (bint)info.internal_free_heap);
    be_data_push(vm, (const char *)"internal_free_heap");
    be_pop(vm, 1);
    be_pushint(vm, (bint)info.internal_min_free_heap);
    be_data_push(vm, (const char *)"internal_min_free_heap");
    be_pop(vm, 1);
    be_pushint(vm, (bint)info.internal_total_heap);
    be_data_push(vm, (const char *)"internal_total_heap");
    be_pop(vm, 1);
    be_pushint(vm, (bint)info.internal_max_alloc_block);
    be_data_push(vm, (const char *)"internal_max_alloc_block");
    be_pop(vm, 1);

    be_return(vm);
}
} // namespace

static bntvmodobj_t const system_attrs[] = {
    be_native_module_function("meminfo", native_system_meminfo),
};

extern "C" const bntvmodule_t be_native_module_system = {
    "system",
    system_attrs,
    sizeof(system_attrs) / sizeof(system_attrs[0]),
    nullptr,
};
