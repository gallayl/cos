#pragma once

#include "berry.h"
#include "berry_runtime.h"

static inline bool berry_json_value(bvm *vm, int index)
{
    return be_isnil(vm, index) || be_isbool(vm, index) || be_isnumber(vm, index) || be_isstring(vm, index) ||
           be_islist(vm, index) || be_islistinstance(vm, index) || be_ismap(vm, index) || be_ismapinstance(vm, index);
}

static inline bool berry_json_call_method(bvm *vm, const char *method, int value_index)
{
    value_index = be_absindex(vm, value_index);
    const int base = be_top(vm);
    if (!berry_runtime_ensure_json(vm) || !be_getglobal(vm, "json") || !be_getmember(vm, -1, method) ||
        be_isnil(vm, -1))
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }

    be_remove(vm, -2);
    be_pushvalue(vm, value_index);
    if (be_pcall(vm, 1) != 0)
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }
    if (be_top(vm) == base + 2)
    {
        be_remove(vm, base + 2);
    }
    if (be_top(vm) != base + 1)
    {
        be_pop(vm, be_top(vm) - base);
        return false;
    }
    return true;
}

static inline bool berry_json_whitespace(char character)
{
    return character == ' ' || character == '\t' || character == '\r' || character == '\n';
}
