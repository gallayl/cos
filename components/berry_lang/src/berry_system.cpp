#include "berry.h"
#include "berry_runtime.h"
#include "esp_err.h"
#include "system.h"
#include "time_sync.h"

#include <cstdint>
#include <ctime>

namespace
{
static void map_set_bool(bvm *vm, const char *key, bool value)
{
    be_pushstring(vm, key);
    be_pushbool(vm, value ? 1 : 0);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

static void map_set_string(bvm *vm, const char *key, const char *value)
{
    be_pushstring(vm, key);
    be_pushstring(vm, value != nullptr ? value : "");
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

static void map_set_int(bvm *vm, const char *key, bint value)
{
    be_pushstring(vm, key);
    be_pushint(vm, value);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
}

static int native_system_meminfo(bvm *vm)
{
    system_info_t info;
    const esp_err_t err = system_get_info(&info);

    be_newobject(vm, "map");
    if (err != ESP_OK)
    {
        map_set_bool(vm, "ok", false);
        map_set_string(vm, "error", esp_err_to_name(err));
        be_pop(vm, 1);
        be_return(vm);
    }

    map_set_bool(vm, "ok", true);
    map_set_int(vm, "free_heap", (bint)info.free_heap);
    map_set_int(vm, "min_free_heap", (bint)info.min_free_heap);
    map_set_int(vm, "total_heap", (bint)info.total_heap);
    map_set_int(vm, "max_alloc_block", (bint)info.max_alloc_block);
    map_set_int(vm, "internal_free_heap", (bint)info.internal_free_heap);
    map_set_int(vm, "internal_min_free_heap", (bint)info.internal_min_free_heap);
    map_set_int(vm, "internal_total_heap", (bint)info.internal_total_heap);
    map_set_int(vm, "internal_max_alloc_block", (bint)info.internal_max_alloc_block);
    be_pop(vm, 1);
    be_return(vm);
}

static int native_system_uptime(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        be_raise(vm, "value_error", "system.uptime() takes no arguments");
        be_return(vm);
    }

    char buf[32];
    const esp_err_t err = system_format_uptime(buf, sizeof(buf));
    if (err != ESP_OK)
    {
        be_raise(vm, "runtime_error", "system.uptime() failed");
        be_return(vm);
    }

    be_pushstring(vm, buf);
    be_return(vm);
}

static int native_system_time_info(bvm *vm)
{
    if (be_top(vm) != 0)
    {
        be_raise(vm, "value_error", "system.time_info() takes no arguments");
        be_return(vm);
    }

    char utc_buf[32];
    char local_buf[32];
    esp_err_t err = time_sync_get_utc(utc_buf, sizeof(utc_buf));
    if (err != ESP_OK)
    {
        be_newobject(vm, "map");
        map_set_bool(vm, "ok", false);
        map_set_string(vm, "error", esp_err_to_name(err));
        be_pop(vm, 1);
        be_return(vm);
    }

    err = time_sync_get_local(local_buf, sizeof(local_buf));
    if (err != ESP_OK)
    {
        be_newobject(vm, "map");
        map_set_bool(vm, "ok", false);
        map_set_string(vm, "error", esp_err_to_name(err));
        be_pop(vm, 1);
        be_return(vm);
    }

    be_newobject(vm, "map");
    map_set_bool(vm, "ok", true);
    map_set_bool(vm, "synced", time_sync_is_synced());
    map_set_int(vm, "epoch", (bint)time_sync_get_epoch());
    map_set_string(vm, "utc", utc_buf);
    map_set_string(vm, "local", local_buf);
    be_pop(vm, 1);
    be_return(vm);
}

static int native_system_sleep(bvm *vm)
{
    if (be_top(vm) != 1 || !be_isint(vm, 1))
    {
        // Invalid argument type/range -> synchronous deterministic error, no pending timer.
        be_raise(vm, "value_error", "system.sleep(ms) requires an integer milliseconds argument");
        be_return(vm);
    }
    const bint ms = be_toint(vm, 1);
    // Berry ints are long; accept negative as <=0 (resolves next pump). Very large values allowed if int.
    be_getglobal(vm, "_cos_system_sleep");
    if (!be_isfunction(vm, -1))
    {
        be_pop(vm, 1);
        be_raise(vm, "runtime_error", "async runtime not initialized");
        be_return(vm);
    }
    be_pushint(vm, ms);
    if (be_pcall(vm, 1) != 0)
    {
        // Propagate Berry exception from helper (should not happen for normal overflow path which rejects)
        const char *error = be_tostring(vm, -1);
        if (error == nullptr)
        {
            error = "system.sleep failed";
        }
        be_raise(vm, "runtime_error", error);
        be_return(vm);
    }
    // be_pcall leaves [result, arg1..argN] with top restored; pop args, then return result.
    be_pop(vm, 1);
    be_return(vm);
}
} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
static bntvmodobj_t const system_attrs[] = {
    be_native_module_function("meminfo", native_system_meminfo),
    be_native_module_function("uptime", native_system_uptime),
    be_native_module_function("time_info", native_system_time_info),
    be_native_module_function("sleep", native_system_sleep),
};

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" const bntvmodule_t be_native_module_system = {
    "system",
    system_attrs,
    sizeof(system_attrs) / sizeof(system_attrs[0]),
    nullptr,
};
