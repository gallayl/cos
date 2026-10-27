#include "ui.h"

#include "display.h"

#include <algorithm>
#include <utility>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

namespace ui
{
namespace
{
Container root;
DisplayRenderer display_renderer;
TouchDispatcher touch_dispatcher;
void (*execution_lock)() = nullptr;
void (*execution_unlock)() = nullptr;
void (*tick_callback)() = nullptr;

std::vector<std::function<void()>> &action_queue()
{
    static std::vector<std::function<void()>> actions;
    return actions;
}

#ifdef ESP_PLATFORM
constexpr char TAG[] = "ui";
TaskHandle_t ui_task_handle = nullptr;

void ui_task(void *)
{
    while (true)
    {
        if (execution_lock != nullptr)
        {
            execution_lock();
        }
        display_start_write();
        display_fill_screen(0xFFFF);
        root.draw(display_renderer);
        display_end_write();
        touch_dispatcher.poll(root);
        if (tick_callback != nullptr)
        {
            tick_callback();
        }
        if (execution_unlock != nullptr)
        {
            execution_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
#endif
} // namespace

void DisplayRenderer::fill_rect(const Bounds &bounds, uint16_t color)
{
    display_fill_rect(bounds.x, bounds.y, bounds.width, bounds.height, color);
}

void DisplayRenderer::draw_hline(int x, int y, int width, uint16_t color)
{
    display_draw_hline(x, y, width, color);
}

void DisplayRenderer::draw_vline(int x, int y, int height, uint16_t color)
{
    display_draw_vline(x, y, height, color);
}

void DisplayRenderer::draw_text(const Bounds &bounds, const char *text, uint16_t foreground, uint16_t background)
{
    display_draw_text(bounds.x, bounds.y, text, foreground, background);
}

void queue_action(std::function<void()> action)
{
    if (action)
    {
        action_queue().push_back(std::move(action));
    }
}

void execute_queued_actions()
{
    auto actions = std::move(action_queue());
    action_queue().clear();
    for (auto &action : actions)
    {
        action();
    }
}

Container &app_root()
{
    return root;
}

bool start()
{
    root.set_bounds({0, 0, display_get_width(), display_get_height()});
    root.mount();
#ifdef ESP_PLATFORM
    if (ui_task_handle != nullptr)
    {
        return true;
    }

    BaseType_t result = xTaskCreate(ui_task, "ui", 4096, nullptr, 5, &ui_task_handle);
    if (result != pdPASS)
    {
        ui_task_handle = nullptr;
        ESP_LOGE(TAG, "Failed to create UI task");
        return false;
    }
#endif
    return true;
}

void set_execution_guard(void (*lock)(), void (*unlock)())
{
    execution_lock = lock;
    execution_unlock = unlock;
}

void set_tick_callback(void (*tick)())
{
    tick_callback = tick;
}

void stop()
{
    root.unmount();
    root.clear();
    touch_dispatcher = TouchDispatcher{};
    action_queue().clear();
}

void Element::set_bounds(const Bounds &bounds)
{
    bounds_ = bounds;
}

const Bounds &Element::bounds() const
{
    return bounds_;
}

bool Element::contains(int x, int y) const
{
    if (bounds_.width <= 0 || bounds_.height <= 0)
    {
        return false;
    }

    const int64_t right = static_cast<int64_t>(bounds_.x) + bounds_.width;
    const int64_t bottom = static_cast<int64_t>(bounds_.y) + bounds_.height;
    return x >= bounds_.x && static_cast<int64_t>(x) < right && y >= bounds_.y && static_cast<int64_t>(y) < bottom;
}

bool Element::is_mounted() const
{
    return mounted_;
}

void Element::mount()
{
    mounted_ = true;
}

void Element::unmount()
{
    mounted_ = false;
}

void Element::on_touch(int, int) {}

void Element::on_touch_end(int, int) {}

Element *Container::add_child(std::unique_ptr<Element> child)
{
    if (!child)
    {
        return nullptr;
    }

    Element *element = child.get();
    if (mounted_)
    {
        element->mount();
    }
    children_.push_back(std::move(child));
    return element;
}

void Container::clear()
{
    if (mounted_)
    {
        for (auto &child : children_)
        {
            child->unmount();
        }
    }
    children_.clear();
}

void Container::mount()
{
    if (mounted_)
    {
        return;
    }

    Element::mount();
    for (auto &child : children_)
    {
        child->mount();
    }
}

void Container::unmount()
{
    if (!mounted_)
    {
        return;
    }

    for (auto &child : children_)
    {
        child->unmount();
    }
    Element::unmount();
}

void Container::draw(Renderer &renderer)
{
    if (!mounted_)
    {
        return;
    }
    for (auto &child : children_)
    {
        child->draw(renderer);
    }
}

void Container::on_touch(int x, int y)
{
    if (!mounted_)
    {
        return;
    }

    for (auto child = children_.rbegin(); child != children_.rend(); ++child)
    {
        if ((*child)->contains(x, y))
        {
            (*child)->on_touch(x, y);
            return;
        }
    }
}

void Container::on_touch_end(int x, int y)
{
    if (!mounted_)
    {
        return;
    }

    for (auto &child : children_)
    {
        child->on_touch_end(x, y);
    }
}

GroupBox::GroupBox(std::string title)
    : title_(std::move(title))
{
}

void GroupBox::set_title(std::string title)
{
    title_ = std::move(title);
}

void GroupBox::set_colors(uint16_t foreground, uint16_t background, uint16_t border)
{
    foreground_ = foreground;
    background_ = background;
    border_ = border;
}

void GroupBox::draw(Renderer &renderer)
{
    if (!mounted_ || bounds_.width <= 0 || bounds_.height <= 0)
    {
        return;
    }

    renderer.fill_rect(bounds_, background_);
    renderer.draw_vline(bounds_.x, bounds_.y, bounds_.height, border_);
    renderer.draw_vline(bounds_.x + bounds_.width - 1, bounds_.y, bounds_.height, border_);
    renderer.draw_hline(bounds_.x, bounds_.y + bounds_.height - 1, bounds_.width, border_);
    constexpr int title_gap = 4;
    constexpr int title_width_per_character = 8;
    int title_width = static_cast<int>(title_.size()) * title_width_per_character;
    title_width = std::min(title_width, std::max(bounds_.width - 12, 0));
    const int left_top_width = std::min(4, bounds_.width);
    renderer.draw_hline(bounds_.x, bounds_.y, left_top_width, border_);
    const int right_start = bounds_.x + left_top_width + title_gap + title_width + title_gap;
    const int right_width = bounds_.x + bounds_.width - right_start;
    if (right_width > 0)
    {
        renderer.draw_hline(right_start, bounds_.y, right_width, border_);
    }

    if (!title_.empty())
    {
        renderer.draw_text({bounds_.x + left_top_width + title_gap, bounds_.y - 4, title_width, 12}, title_.c_str(),
                           foreground_, background_);
    }
    Container::draw(renderer);
}

Label::Label(std::string text)
    : text_(std::move(text))
{
}

void Label::set_text(std::string text)
{
    text_ = std::move(text);
}

const std::string &Label::text() const
{
    return text_;
}

void Label::set_colors(uint16_t foreground, uint16_t background)
{
    foreground_ = foreground;
    background_ = background;
}

void Label::draw(Renderer &renderer)
{
    if (mounted_)
    {
        renderer.draw_text(bounds_, text_.c_str(), foreground_, background_);
    }
}

Checkbox::Checkbox(std::string text)
    : text_(std::move(text))
{
}

void Checkbox::set_text(std::string text)
{
    text_ = std::move(text);
}

const std::string &Checkbox::text() const
{
    return text_;
}

void Checkbox::set_checked(bool checked)
{
    checked_ = checked;
}

bool Checkbox::is_checked() const
{
    return checked_;
}

void Checkbox::set_on_change(Callback callback)
{
    on_change_ = std::move(callback);
}

void Checkbox::draw(Renderer &renderer)
{
    if (!mounted_ || bounds_.width <= 0 || bounds_.height <= 0)
    {
        return;
    }

    const int box_size = std::min(12, bounds_.height);
    const int box_y = bounds_.y + (bounds_.height - box_size) / 2;
    renderer.fill_rect({bounds_.x, box_y, box_size, box_size}, background_);
    renderer.draw_hline(bounds_.x, box_y, box_size, foreground_);
    renderer.draw_hline(bounds_.x, box_y + box_size - 1, box_size, foreground_);
    renderer.draw_vline(bounds_.x, box_y, box_size, foreground_);
    renderer.draw_vline(bounds_.x + box_size - 1, box_y, box_size, foreground_);
    if (checked_ && box_size >= 6)
    {
        renderer.draw_hline(bounds_.x + 3, box_y + box_size / 2, box_size - 6, foreground_);
    }

    const int text_x = bounds_.x + box_size + 4;
    const int text_width = bounds_.x + bounds_.width - text_x;
    if (text_width > 0)
    {
        renderer.draw_text({text_x, bounds_.y, text_width, bounds_.height}, text_.c_str(), foreground_, background_);
    }
}

void Checkbox::on_touch(int x, int y)
{
    pressed_ = mounted_ && contains(x, y);
}

void Checkbox::on_touch_end(int x, int y)
{
    const bool clicked = pressed_ && mounted_ && contains(x, y);
    pressed_ = false;
    if (clicked)
    {
        checked_ = !checked_;
        if (on_change_)
        {
            queue_action([callback = on_change_, checked = checked_]() { callback(checked); });
        }
    }
}

Button::Button(std::string text)
    : text_(std::move(text))
{
}

void Button::set_text(std::string text)
{
    text_ = std::move(text);
}

void Button::set_on_click(Callback callback)
{
    on_click_ = std::move(callback);
}

bool Button::is_pressed() const
{
    return pressed_;
}

void Button::unmount()
{
    pressed_ = false;
    Element::unmount();
}

void Button::draw(Renderer &renderer)
{
    if (!mounted_ || bounds_.width <= 0 || bounds_.height <= 0)
    {
        return;
    }

    constexpr uint16_t light = 0xFFFF;
    constexpr uint16_t dark = 0x0000;
    const uint16_t top_left = pressed_ ? dark : light;
    const uint16_t bottom_right = pressed_ ? light : dark;

    renderer.fill_rect(bounds_, background_);
    renderer.draw_hline(bounds_.x, bounds_.y, bounds_.width, top_left);
    renderer.draw_vline(bounds_.x, bounds_.y, bounds_.height, top_left);
    renderer.draw_hline(bounds_.x, bounds_.y + bounds_.height - 1, bounds_.width, bottom_right);
    renderer.draw_vline(bounds_.x + bounds_.width - 1, bounds_.y, bounds_.height, bottom_right);
    renderer.draw_text(bounds_, text_.c_str(), 0xFFFF, background_);
}

void Button::on_touch(int x, int y)
{
    pressed_ = mounted_ && contains(x, y);
}

void Button::on_touch_end(int x, int y)
{
    const bool clicked = pressed_ && mounted_ && contains(x, y);
    pressed_ = false;
    if (clicked && on_click_)
    {
        queue_action(on_click_);
    }
}

void TouchDispatcher::update(Element &root, bool is_down, int x, int y)
{
    if (is_down && !was_down_)
    {
        root.on_touch(x, y);
    }
    else if (!is_down && was_down_)
    {
        root.on_touch_end(x, y);
        execute_queued_actions();
    }

    if (is_down)
    {
        last_x_ = x;
        last_y_ = y;
    }
    was_down_ = is_down;
}

void TouchDispatcher::poll(Element &root)
{
    int x = 0;
    int y = 0;
    const bool is_down = display_get_touch(&x, &y);
    update(root, is_down, is_down ? x : last_x_, is_down ? y : last_y_);
}

} // namespace ui
