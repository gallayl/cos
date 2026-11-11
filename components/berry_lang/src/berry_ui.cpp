#include "berry_runtime.h"

#include "berry.h"
#include "berry_internal.h"
#include "display.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "ui.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace
{
enum class ElementType : std::uint8_t // NOLINT(performance-enum-size,readability-identifier-naming) - local enum
                                      // values/style kept for consistency
{
    kRoot,     // NOLINT(readability-identifier-naming)
    kLabel,    // NOLINT(readability-identifier-naming)
    kButton,   // NOLINT(readability-identifier-naming)
    kGroupBox, // NOLINT(readability-identifier-naming)
    kCheckbox  // NOLINT(readability-identifier-naming)
};

struct UiHandle // NOLINT(readability-identifier-naming) - local UI handle type used consistently in this translation
                // unit

{
    ui::Element *element;
    ElementType type;
    int parent;
    int click_callback;
    int change_callback;
};

bvm *s_vm = nullptr;
bool s_app_active = false;
bool s_key_down[128] = {};
int s_next_callback = 0;
std::vector<UiHandle> s_handles;
std::vector<std::string> s_callback_globals;

struct UiTimer // NOLINT(readability-identifier-naming) - local UI timer type used consistently in this translation unit
{
    int callback_id;
    int64_t interval_us;
    int64_t due_us;
};

std::vector<UiTimer> s_timers;

UiHandle *get_handle(int value)
{
    if (value < 0 || static_cast<size_t>(value) >= s_handles.size())
    {
        return nullptr;
    }

    UiHandle &handle = s_handles[static_cast<size_t>(value)];
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
        be_pushbool(s_vm, value ? 1 : 0);
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

    const UiHandle *parent = get_handle(handle);
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
        const UiHandle *parent = get_handle(parent_handle);
        if (parent->type != ElementType::kGroupBox)
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
    return add_element(vm, ElementType::kLabel, std::make_unique<ui::Label>(text != nullptr ? text : ""),
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
    return add_element(vm, ElementType::kButton, std::make_unique<ui::Button>(text != nullptr ? text : ""),
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
    return add_element(vm, ElementType::kGroupBox, std::make_unique<ui::GroupBox>(title != nullptr ? title : ""),
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
    return add_element(vm, ElementType::kCheckbox, std::make_unique<ui::Checkbox>(text != nullptr ? text : ""),
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
    const UiHandle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    const char *text = be_tostring(vm, 2);
    if (handle != nullptr && text != nullptr)
    {
        if (handle->type == ElementType::kLabel)
        {
            static_cast<ui::Label *>(handle->element)->set_text(text);
        }
        else if (handle->type == ElementType::kButton)
        {
            static_cast<ui::Button *>(handle->element)->set_text(text);
        }
        else if (handle->type == ElementType::kCheckbox)
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
    const UiHandle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    const uint16_t foreground = static_cast<uint16_t>(be_toint(vm, 2));
    const uint16_t background = be_top(vm) >= 3 ? static_cast<uint16_t>(be_toint(vm, 3)) : 0xFFFF;
    if (handle != nullptr && handle->type == ElementType::kLabel)
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
    const UiHandle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
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
    const UiHandle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
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
    UiHandle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    if (handle != nullptr && handle->type == ElementType::kButton)
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
    UiHandle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    if (handle != nullptr && handle->type == ElementType::kCheckbox)
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
    const UiHandle *handle = get_handle(static_cast<int>(be_toint(vm, 1)));
    if (handle != nullptr && handle->type == ElementType::kCheckbox)
    {
        static_cast<ui::Checkbox *>(handle->element)->set_checked(be_tobool(vm, 2));
    }
    be_return_nil(vm);
}

int native_get_checked(bvm *vm)
{
    const UiHandle *handle = be_top(vm) >= 1 ? get_handle(static_cast<int>(be_toint(vm, 1))) : nullptr;
    if (handle != nullptr && handle->type == ElementType::kCheckbox)
    {
        be_pushbool(vm, static_cast<ui::Checkbox *>(handle->element)->is_checked() ? 1 : 0);
        be_return(vm);
    }
    be_pushbool(vm, 0);
    be_return(vm);
}

int native_clear(bvm *vm)
{
    const UiHandle *handle = be_top(vm) >= 1 ? get_handle(static_cast<int>(be_toint(vm, 1))) : nullptr;
    if (handle != nullptr && handle->type == ElementType::kGroupBox)
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
    else if (handle != nullptr && handle->type == ElementType::kRoot)
    {
        ui::app_root().clear();
        for (const UiHandle &entry : s_handles)
        {
            remove_callback(entry.click_callback);
            remove_callback(entry.change_callback);
        }
        s_handles.assign(1, {&ui::app_root(), ElementType::kRoot, -1, -1, -1});
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

int native_launch(bvm *vm)
{
    if (be_top(vm) < 1)
    {
        be_raise(vm, "value_error", "ui.launch(app) requires an app class or instance");
    }

    const esp_err_t err = berry_runtime_ui_launch(vm, 1);
    if (err != ESP_OK)
    {
        be_raise(vm, "runtime_error", "ui.launch failed");
    }
    be_pushint(vm, 0);
    be_return(vm);
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
    display_draw_text(static_cast<int>(be_toint(vm, 1)), static_cast<int>(be_toint(vm, 2)), text != nullptr ? text : "",
                      static_cast<uint16_t>(be_toint(vm, 4)), static_cast<uint16_t>(be_toint(vm, 5)));
    be_return_nil(vm);
}

int native_key_down(bvm *vm)
{
    if (be_top(vm) < 1 || !be_isint(vm, 1))
    {
        be_pushbool(vm, 0);
        be_return(vm);
    }

    const int keycode = static_cast<int>(be_toint(vm, 1));
    const bool down =
        keycode > 0 && keycode < static_cast<int>(sizeof(s_key_down) / sizeof(s_key_down[0])) && s_key_down[keycode];
    be_pushbool(vm, down ? 1 : 0);
    be_return(vm);
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

int native_ui_init(bvm *vm)
{
    s_vm = vm;
    be_pushvalue(vm, 1);
    be_return(vm);
}
} // namespace

// NOLINTNEXTLINE(misc-use-internal-linkage) - referenced from other translation units
void berry_ui_tick(void)
{
    berry_ui_tick_impl();
}

// NOLINTNEXTLINE(readability-identifier-naming) - Berry module table naming convention
static bntvmodobj_t const ui_attrs[] = {
    be_native_module_function("init", native_ui_init),
    be_native_module_function("label", native_label),
    be_native_module_function("button", native_button),
    be_native_module_function("groupbox", native_groupbox),
    be_native_module_function("checkbox", native_checkbox),
    be_native_module_function("set_text", native_set_text),
    be_native_module_function("set_text_color", native_set_text_color),
    be_native_module_function("set_bounds", native_set_bounds),
    be_native_module_function("bounds", native_bounds),
    be_native_module_function("on_click", native_on_click),
    be_native_module_function("on_change", native_on_change),
    be_native_module_function("set_checked", native_set_checked),
    be_native_module_function("get_checked", native_get_checked),
    be_native_module_function("clear", native_clear),
    be_native_module_function("color565", native_color565),
    be_native_module_function("mark_dirty", native_mark_dirty),
    be_native_module_function("random", native_random),
    be_native_module_function("exit", native_exit),
    be_native_module_function("launch", native_launch),
    be_native_module_function("fill_screen", native_fill_screen),
    be_native_module_function("fill_rect", native_fill_rect),
    be_native_module_function("draw_text", native_draw_text),
    be_native_module_function("touch", native_touch),
    be_native_module_function("key_down", native_key_down),
    be_native_module_function("start_write", native_start_write),
    be_native_module_function("end_write", native_end_write),
    be_native_module_function("timer", native_timer),
    be_native_module_int("LEFT", 1),
    be_native_module_int("CENTER", 0),
    be_native_module_int("RIGHT", 2),
    be_native_module_int("KEY_ENTER", 0x28),
    be_native_module_int("KEY_ESCAPE", 0x29),
    be_native_module_int("KEY_SPACE", 0x2C),
    be_native_module_int("KEY_A", 0x04),
    be_native_module_int("KEY_D", 0x07),
    be_native_module_int("KEY_LEFT", 0x50),
    be_native_module_int("KEY_RIGHT", 0x4F),
    be_native_module_int("KEY_DOWN", 0x51),
    be_native_module_int("KEY_UP", 0x52),
    be_native_module_int("TEXT_COLOR", 0x0000),
    be_native_module_int("WINDOW_BG", 0xFFFF),
    be_native_module_int("BUTTON_HIGHLIGHT", 0xFFFF),
    be_native_module_int("BUTTON_SHADOW", 0x0000),
};

// NOLINTNEXTLINE(readability-identifier-naming) - Berry native module symbol name
extern "C" const bntvmodule_t be_native_module_ui = {
    "ui",
    ui_attrs,
    sizeof(ui_attrs) / sizeof(ui_attrs[0]),
    nullptr,
};

// NOLINTNEXTLINE(misc-use-internal-linkage) - referenced from other translation units
void berry_ui_begin_app(bvm *vm)
{
    s_vm = vm;
    s_app_active = true;
    std::fill(s_key_down, s_key_down + sizeof(s_key_down) / sizeof(s_key_down[0]), false);
    s_next_callback = 0;
    s_timers.clear();
    s_callback_globals.clear();
    s_handles.assign(1, {&ui::app_root(), ElementType::kRoot, -1, -1, -1});
}

// NOLINTNEXTLINE(misc-use-internal-linkage) - referenced from other translation units
void berry_ui_end_app(void)
{
    s_app_active = false;
    std::fill(s_key_down, s_key_down + sizeof(s_key_down) / sizeof(s_key_down[0]), false);
    if (s_vm != nullptr)
    {
        for (const std::string &name : s_callback_globals)
        {
            be_pushnil(s_vm);
            be_setglobal(s_vm, name.c_str());
            be_pop(s_vm, 1);
        }
    }
    s_callback_globals.clear();
    s_timers.clear();
    s_handles.clear();
    s_next_callback = 0;
}

// NOLINTNEXTLINE(misc-use-internal-linkage) - referenced from other translation units
void berry_ui_set_key_state(uint8_t keycode, bool pressed)
{
    if (s_app_active && keycode > 0 && keycode < sizeof(s_key_down) / sizeof(s_key_down[0]))
    {
        s_key_down[keycode] = pressed;
    }
}
