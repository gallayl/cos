#include "berry.h"

be_extern_native_module(string);
be_extern_native_module(json);
be_extern_native_module(math);
be_extern_native_module(global);
be_extern_native_module(gc);
be_extern_native_module(introspect);
be_extern_native_module(undefined);

BERRY_LOCAL const bntvmodule_t *const be_module_table[] = {
#if BE_USE_STRING_MODULE
    &be_native_module(string),
#endif
#if BE_USE_JSON_MODULE
    &be_native_module(json),
#endif
#if BE_USE_MATH_MODULE
    &be_native_module(math),
#endif
#if BE_USE_GLOBAL_MODULE
    &be_native_module(global),
#endif
#if BE_USE_GC_MODULE
    &be_native_module(gc),
#endif
#if BE_USE_INTROSPECT_MODULE
    &be_native_module(introspect),
#endif
    &be_native_module(undefined),  NULL,
};

BERRY_LOCAL bclass_array be_class_table = {
    NULL,
};
