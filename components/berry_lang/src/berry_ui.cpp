#include "berry_runtime.h"

#include "berry.h"
#include "berry_internal.h"
#include "esp_random.h"
#include "display.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ui.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace
{
enum class ElementType
{
    Root,
    Label,
    Button,
    GroupBox,
    Checkbox
};

struct Handle
{
    ui::Element *element;
    ElementType type;
    int parent;
    int click_callback;
    int change_callback;
};

bvm *s_vm = nullptr;
bool s_app_active = false;
int s_next_callback = 0;
std::vector<Handle> s_handles;
std::vector<std::string> s_callback_globals;

struct Timer
{
    int callback_id;
    int64_t interval_us;
    int64_t due_us;
};

std::vector<Timer> s_timers;

Handle *get_handle(int value)
{
    if (value < 0 || static_cast<size_t>(value) >= s_handles.size())
    {
        return nullptr;
    }

    Handle &handle = s_handles[static_cast<size_t>(value)];
    return handle.element != nullptr ? &handle : nullptr;
}

int store_callback(bvm *vm, int stack_index)
{
    const int id = s_next_callback++;
    const std::string name = "_cos_ui_callback_" + std::to_string(id);
    be_pushvalue(vm, stack_index);
    be_setglobal(vm, name.c_str());
    be_pop(vm, 1);
    s_callback_globals.push_back(name);
    return id;
}

void remove_callback(int callback_id)
{
    if (callback_id < 0 || s_vm == nullptr)
    {
        return;
    }

    const std::string name = "_cos_ui_callback_" + std::to_string(callback_id);
    be_pushnil(s_vm);
    be_setglobal(s_vm, name.c_str());
    be_pop(s_vm, 1);
    auto callback = std::find(s_callback_globals.begin(), s_callback_globals.end(), name);
    if (callback != s_callback_globals.end())
    {
        s_callback_globals.erase(callback);
    }
}

void invoke_callback(int callback_id, bool has_bool, bool value)
{
    berry_runtime_lock();
    if (s_vm == nullptr)
    {
        berry_runtime_unlock();
        return;
    }

    const std::string name = "_cos_ui_callback_" + std::to_string(callback_id);
    be_getglobal(s_vm, name.c_str());
    if (!be_isfunction(s_vm, -1))
    {
        be_pop(s_vm, 1);
        berry_runtime_unlock();
        return;
    }

    int argc = 0;
    if (has_bool)
    {
        be_pushbool(s_vm, value);
        argc = 1;
    }
    if (be_pcall(s_vm, argc) != 0)
    {
        const char *error = be_tostring(s_vm, -1);
        ESP_LOGE("berry-ui", "Callback failed: %s", error != nullptr ? error : "unknown Berry error");
    }
    be_pop(s_vm, argc + 1);
    berry_runtime_unlock();
}

bool parent_bounds(int handle, int &x, int &y)
{
    if (handle == 0)
    {
        x = 0;
        y = 0;
        return true;
    }

    Handle *parent = get_handle(handle);
    if (parent == nullptr || parent->element == nullptr)
    {
        return false;
    }

    const ui::Bounds &bounds = parent->element->bounds();
    x = bounds.x;
    y = bounds.y;
    return true;
}

template <typename ElementTypeT>
int add_element(bvm *vm, ElementType type, std::unique_ptr<ElementTypeT> element, int parent_handle, int x, int y,
                int width, int height)
{
    if (!s_app_active || element == nullptr)
    {
        be_pushnil(vm);
        be_return(vm);
    }

    int parent_x = 0;
    int parent_y = 0;
    if (!parent_bounds(parent_handle, parent_x, parent_y))
    {
        be_pushnil(vm);
        be_return(vm);
    }

    element->set_bounds({parent_x + x, parent_y + y, width, height});
    ui::Element *raw = element.get();
    if (parent_handle == 0)
    {
        ui::app_root().add_child(std::move(element));
    }
    else
    {
        Handle *parent = get_handle(parent_handle);
        if (parent->type != ElementType::GroupBox)
        {
            be_pushnil(vm);
            be_return(vm);
        }
        static_cast<ui::GroupBox *>(parent->element)->add_child(std::move(element));
    }

    s_handles.push_back({raw, type, parent_handle, -1, -1});
    be_pushint(vm, static_cast<int>(s_handles.size() - 1));
    be_return(vm);
}

int native_label(bvm *vm)
{
    if (be_top(vm) < 6 || !be_isstring(vm, 2))
    {
        be_pushnil(vm);
        be_return(vm);
    }
    const char *text = be_tostring(vm, 2);
    return add_element(vm, ElementType::Label, std::make_unique<ui::Label>(text != nullptr ? text : ""),
                       static_cast<int>(be_toint(vm, 1)), static_cast<int>(be_toint(vm, 3)),
                       static_cast<int>(be_toint(vm, 4)), static_cast<int>(be_toint(vm, 5)),
                       static_cast<int>(be_toint(vm, 6)));
}

int native_button(bvm *vm)
{
    if (be_top(vm) < 6 || !be_isstring(vm, 2))
    {
        be_pushnil(vm);
        be_return(vm);
    }
    const char *text = be_tostring(vm, 2);
    return add_element(vm, ElementType::Button, std::make_unique<ui::Button>(text != nullptr ? text : ""),
                       static_cast<int>(be_toint(vm, 1)), static_cast<int>(be_toint(vm, 3)),
                       static_cast<int>(be_toint(vm, 4)), static_cast<int>(be_toint(vm, 5)),
                       static_cast<int>(be_toint(vm, 6)));
}

int native_groupbox(bvm *vm)
{
    if (be_top(vm) < 6 || !be_isstring(vm, 2))
    {
        be_pushnil(vm);
        be_return(vm);
    }
    const char *title = be_tostring(vm, 2);
    return add_element(vm, ElementType::GroupBox, std::make_unique<ui::GroupBox>(title != nullptr ? title : ""),
                       static_cast<int>(be_toint(vm, 1)), static_cast<int>(be_toint(vm, 3)),
                       static_cast<int>(be_toint(vm, 4)), static_cast<int>(be_toint(vm, 5)),
                       static_cast<int>(be_toint(vm, 6)));
}

int native_checkbox(bvm *vm)
{
    if (be_top(vm) < 6 || !be_isstring(vm, 2))
    {
        be_pushnil(vm);
        be_return(vm);
    }
    const char *text = be_tostring(vm, 2);
    return add_element(vm, ElementType::Checkbox, std::make_unique<ui::Checkbox>(text != nullptr ? text : ""),
                       static_cast<int>(be_toint(vm, 1)), static_cast<int>(be_toint(vm, 3)),
                       static_cast<int>(be_toint(vm, 4)), static_cast<int>(be_toint(vm, 5)),
                       static_cast<int>(be_toint(vm, 6)));
}

int native_set_text(bvm *vm)
{
    if (be_top(vm) < 2 || !be_isstring(vm, 2))
    {
        be_return_nil(vm);
    }
    Handle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    const char *text = be_tostring(vm, 2);
    if (handle != nullptr && text != nullptr)
    {
        if (handle->type == ElementType::Label)
        {
            static_cast<ui::Label *>(handle->element)->set_text(text);
        }
        else if (handle->type == ElementType::Button)
        {
            static_cast<ui::Button *>(handle->element)->set_text(text);
        }
        else if (handle->type == ElementType::Checkbox)
        {
            static_cast<ui::Checkbox *>(handle->element)->set_text(text);
        }
    }
    be_return_nil(vm);
}

int native_set_text_color(bvm *vm)
{
    if (be_top(vm) < 2)
    {
        be_return_nil(vm);
    }
    Handle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    const uint16_t foreground = static_cast<uint16_t>(be_toint(vm, 2));
    const uint16_t background = be_top(vm) >= 3 ? static_cast<uint16_t>(be_toint(vm, 3)) : 0xFFFF;
    if (handle != nullptr && handle->type == ElementType::Label)
    {
        static_cast<ui::Label *>(handle->element)->set_colors(foreground, background);
    }
    be_return_nil(vm);
}

int native_set_bounds(bvm *vm)
{
    if (be_top(vm) < 5)
    {
        be_return_nil(vm);
    }
    Handle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    if (handle != nullptr)
    {
        const ui::Bounds old_bounds = handle->element->bounds();
        if (handle->element->is_mounted() && old_bounds.width > 0 && old_bounds.height > 0)
        {
            display_fill_rect(old_bounds.x, old_bounds.y, old_bounds.width, old_bounds.height, 0xFFFF);
        }
        handle->element->set_bounds({static_cast<int>(be_toint(vm, 2)), static_cast<int>(be_toint(vm, 3)),
                                     static_cast<int>(be_toint(vm, 4)), static_cast<int>(be_toint(vm, 5))});
    }
    be_return_nil(vm);
}

int native_bounds(bvm *vm)
{
    if (be_top(vm) < 1)
    {
        be_return_nil(vm);
    }
    Handle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    if (handle == nullptr)
    {
        be_return_nil(vm);
    }

    const ui::Bounds &bounds = handle->element->bounds();
    be_newlist(vm);
    be_pushint(vm, bounds.x);
    be_data_push(vm, -2);
    be_pushint(vm, bounds.y);
    be_data_push(vm, -2);
    be_pushint(vm, bounds.width);
    be_data_push(vm, -2);
    be_pushint(vm, bounds.height);
    be_data_push(vm, -2);
    be_return(vm);
}

int native_on_click(bvm *vm)
{
    if (be_top(vm) < 2 || !be_isfunction(vm, 2))
    {
        be_return_nil(vm);
    }
    Handle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    if (handle != nullptr && handle->type == ElementType::Button)
    {
        remove_callback(handle->click_callback);
        const int callback_id = store_callback(vm, 2);
        handle->click_callback = callback_id;
        static_cast<ui::Button *>(handle->element)
            ->set_on_click([callback_id]() { invoke_callback(callback_id, false, false); });
    }
    be_return_nil(vm);
}

int native_on_change(bvm *vm)
{
    if (be_top(vm) < 2 || !be_isfunction(vm, 2))
    {
        be_return_nil(vm);
    }
    Handle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    if (handle != nullptr && handle->type == ElementType::Checkbox)
    {
        remove_callback(handle->change_callback);
        const int callback_id = store_callback(vm, 2);
        handle->change_callback = callback_id;
        static_cast<ui::Checkbox *>(handle->element)
            ->set_on_change([callback_id](bool value) { invoke_callback(callback_id, true, value); });
    }
    be_return_nil(vm);
}

int native_set_checked(bvm *vm)
{
    if (be_top(vm) < 2)
    {
        be_return_nil(vm);
    }
    Handle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    if (handle != nullptr && handle->type == ElementType::Checkbox)
    {
        static_cast<ui::Checkbox *>(handle->element)->set_checked(be_tobool(vm, 2));
    }
    be_return_nil(vm);
}

int native_get_checked(bvm *vm)
{
    Handle *handle = be_top(vm) >= 1 ? get_handle(static_cast<int>(be_toint(vm, 1))) : nullptr;
    if (handle != nullptr && handle->type == ElementType::Checkbox)
    {
        be_pushbool(vm, static_cast<ui::Checkbox *>(handle->element)->is_checked());
        be_return(vm);
    }
    be_pushbool(vm, false);
    be_return(vm);
}

int native_clear(bvm *vm)
{
    Handle *handle = be_top(vm) >= 1 ? get_handle(static_cast<int>(be_toint(vm, 1))) : nullptr;
    if (handle != nullptr && handle->type == ElementType::GroupBox)
    {
        auto *group = static_cast<ui::GroupBox *>(handle->element);
        group->clear();
        for (size_t index = 0; index < s_handles.size(); ++index)
        {
            int parent = s_handles[index].parent;
            while (parent > 0 && static_cast<size_t>(parent) < s_handles.size())
            {
                if (parent == static_cast<int>(be_toint(vm, 1)))
                {
                    remove_callback(s_handles[index].click_callback);
                    remove_callback(s_handles[index].change_callback);
                    s_handles[index].element = nullptr;
                    break;
                }
                parent = s_handles[static_cast<size_t>(parent)].parent;
            }
        }
    }
    else if (handle != nullptr && handle->type == ElementType::Root)
    {
        ui::app_root().clear();
        for (Handle &entry : s_handles)
        {
            remove_callback(entry.click_callback);
            remove_callback(entry.change_callback);
        }
        s_handles.assign(1, {&ui::app_root(), ElementType::Root, -1, -1, -1});
    }
    ui::invalidate();
    be_return_nil(vm);
}

int native_color565(bvm *vm)
{
    if (be_top(vm) < 3)
    {
        be_return_nil(vm);
    }
    const uint16_t color = static_cast<uint16_t>(((be_toint(vm, 1) & 0xF8) << 8) | ((be_toint(vm, 2) & 0xFC) << 3) |
                                                 (be_toint(vm, 3) >> 3));
    be_pushint(vm, color);
    be_return(vm);
}

int native_mark_dirty(bvm *vm)
{
    (void)vm;
    ui::invalidate();
    be_return_nil(vm);
}

int native_random(bvm *vm)
{
    if (be_top(vm) < 1)
    {
        be_return_nil(vm);
    }

    const int limit = static_cast<int>(be_toint(vm, 1));
    if (limit <= 0)
    {
        be_return_nil(vm);
    }

    be_pushint(vm, static_cast<int>(esp_random() % static_cast<uint32_t>(limit)));
    be_return(vm);
}

int native_exit(bvm *vm)
{
    (void)vm;
    ui::request_exit();
    be_return_nil(vm);
}

int native_fill_screen(bvm *vm)
{
    if (be_top(vm) < 1)
    {
        be_return_nil(vm);
    }

    ui::preserve_display_on_start();
    display_fill_screen(static_cast<uint16_t>(be_toint(vm, 1)));
    be_return_nil(vm);
}

int native_fill_rect(bvm *vm)
{
    if (be_top(vm) < 5)
    {
        be_return_nil(vm);
    }

    display_fill_rect(static_cast<int>(be_toint(vm, 1)), static_cast<int>(be_toint(vm, 2)),
                      static_cast<int>(be_toint(vm, 3)), static_cast<int>(be_toint(vm, 4)),
                      static_cast<uint16_t>(be_toint(vm, 5)));
    be_return_nil(vm);
}

int native_draw_text(bvm *vm)
{
    if (be_top(vm) < 5 || !be_isstring(vm, 3))
    {
        be_return_nil(vm);
    }

    const char *text = be_tostring(vm, 3);
    display_draw_text(static_cast<int>(be_toint(vm, 1)), static_cast<int>(be_toint(vm, 2)),
                      text != nullptr ? text : "", static_cast<uint16_t>(be_toint(vm, 4)),
                      static_cast<uint16_t>(be_toint(vm, 5)));
    be_return_nil(vm);
}

int native_touch(bvm *vm)
{
    int x = 0;
    int y = 0;
    if (!display_get_touch(&x, &y))
    {
        be_pushnil(vm);
        be_return(vm);
    }

    be_newobject(vm, "list");
    be_pushint(vm, x);
    be_data_push(vm, -2);
    be_pop(vm, 1);
    be_pushint(vm, y);
    be_data_push(vm, -2);
    be_pop(vm, 1);
    be_pop(vm, 1);
    be_return(vm);
}

int native_start_write(bvm *vm)
{
    (void)vm;
    display_start_write();
    be_return_nil(vm);
}

int native_end_write(bvm *vm)
{
    (void)vm;
    display_end_write();
    display_wait();
    be_return_nil(vm);
}

int native_timer(bvm *vm)
{
    if (be_top(vm) < 2 || !be_isfunction(vm, 2))
    {
        be_return_nil(vm);
    }

    const int64_t interval_us = static_cast<int64_t>(be_toint(vm, 1)) * 1000;
    if (interval_us <= 0)
    {
        be_return_nil(vm);
    }
    const int callback_id = store_callback(vm, 2);
    s_timers.push_back({callback_id, interval_us, esp_timer_get_time() + interval_us});
    be_return_nil(vm);
}

void berry_ui_tick_impl()
{
    const int64_t now = esp_timer_get_time();
    const size_t timer_count = s_timers.size();
    for (size_t index = 0; index < timer_count; ++index)
    {
        if (now >= s_timers[index].due_us)
        {
            const int callback_id = s_timers[index].callback_id;
            const int64_t interval_us = s_timers[index].interval_us;
            s_timers[index].due_us = now + interval_us;
            invoke_callback(callback_id, false, false);
        }
    }
}

void register_function(bvm *vm, const char *name, bntvfunc function)
{
    be_pushntvfunction(vm, function);
    be_setmember(vm, -2, name);
    be_pop(vm, 1);
}

void register_constant(bvm *vm, const char *name, int value)
{
    be_pushint(vm, value);
    be_setmember(vm, -2, name);
    be_pop(vm, 1);
}
} // namespace

void berry_ui_tick(void)
{
    berry_ui_tick_impl();
}

void berry_ui_register(bvm *vm)
{
    s_vm = vm;
    be_newmodule(vm);
    be_setname(vm, -1, "ui");
    register_function(vm, "label", native_label);
    register_function(vm, "button", native_button);
    register_function(vm, "groupbox", native_groupbox);
    register_function(vm, "checkbox", native_checkbox);
    register_function(vm, "set_text", native_set_text);
    register_function(vm, "set_text_color", native_set_text_color);
    register_function(vm, "set_bounds", native_set_bounds);
    register_function(vm, "bounds", native_bounds);
    register_function(vm, "on_click", native_on_click);
    register_function(vm, "on_change", native_on_change);
    register_function(vm, "set_checked", native_set_checked);
    register_function(vm, "get_checked", native_get_checked);
    register_function(vm, "clear", native_clear);
    register_function(vm, "color565", native_color565);
    register_function(vm, "mark_dirty", native_mark_dirty);
    register_function(vm, "random", native_random);
    register_function(vm, "exit", native_exit);
    register_function(vm, "fill_screen", native_fill_screen);
    register_function(vm, "fill_rect", native_fill_rect);
    register_function(vm, "draw_text", native_draw_text);
    register_function(vm, "touch", native_touch);
    register_function(vm, "start_write", native_start_write);
    register_function(vm, "end_write", native_end_write);
    register_function(vm, "timer", native_timer);
    register_constant(vm, "LEFT", 1);
    register_constant(vm, "CENTER", 0);
    register_constant(vm, "RIGHT", 2);
    register_constant(vm, "TEXT_COLOR", 0x0000);
    register_constant(vm, "WINDOW_BG", 0xFFFF);
    register_constant(vm, "BUTTON_HIGHLIGHT", 0xFFFF);
    register_constant(vm, "BUTTON_SHADOW", 0x0000);
    be_setglobal(vm, "ui");
    be_pop(vm, 1);
}

void berry_ui_begin_app(bvm *vm)
{
    s_vm = vm;
    s_app_active = true;
    s_next_callback = 0;
    s_timers.clear();
    s_callback_globals.clear();
    s_handles.assign(1, {&ui::app_root(), ElementType::Root, -1, -1, -1});
}

void berry_ui_end_app(void)
{
    s_app_active = false;
    for (const std::string &name : s_callback_globals)
    {
        be_pushnil(s_vm);
        be_setglobal(s_vm, name.c_str());
        be_pop(s_vm, 1);
    }
    s_callback_globals.clear();
    s_timers.clear();
    s_handles.clear();
    s_next_callback = 0;
}
