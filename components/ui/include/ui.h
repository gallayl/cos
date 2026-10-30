#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ui
{

struct Bounds
{
    int x;
    int y;
    int width;
    int height;
};

class Container;

class Renderer
{
  public:
    virtual ~Renderer() = default;
    virtual void fill_rect(const Bounds &bounds, uint16_t color) = 0;
    virtual void draw_hline(int x, int y, int width, uint16_t color) = 0;
    virtual void draw_vline(int x, int y, int height, uint16_t color) = 0;
    virtual void draw_text(const Bounds &bounds, const char *text, uint16_t foreground, uint16_t background) = 0;
};

class DisplayRenderer final : public Renderer
{
  public:
    void fill_rect(const Bounds &bounds, uint16_t color) override;
    void draw_hline(int x, int y, int width, uint16_t color) override;
    void draw_vline(int x, int y, int height, uint16_t color) override;
    void draw_text(const Bounds &bounds, const char *text, uint16_t foreground, uint16_t background) override;
};

void queue_action(std::function<void()> action);
void execute_queued_actions();
void invalidate(bool clear_background = false);
void preserve_display_on_start();
void set_exit_callback(void (*callback)());
void request_exit();
Container &app_root();
bool start();
void stop();
void set_execution_guard(void (*lock)(), void (*unlock)());
void set_tick_callback(void (*tick)());

class Element
{
  public:
    virtual ~Element() = default;

    void set_bounds(const Bounds &bounds);
    const Bounds &bounds() const;
    bool contains(int x, int y) const;
    bool is_mounted() const;

    virtual void mount();
    virtual void unmount();
    virtual void draw(Renderer &renderer) = 0;
    virtual void on_touch(int x, int y);
    virtual void on_touch_end(int x, int y);

  protected:
    Bounds bounds_{0, 0, 0, 0};
    bool mounted_{false};
};

class Container : public Element
{
  public:
    Element *add_child(std::unique_ptr<Element> child);
    void clear();
    void mount() override;
    void unmount() override;
    void draw(Renderer &renderer) override;
    void on_touch(int x, int y) override;
    void on_touch_end(int x, int y) override;

  private:
    std::vector<std::unique_ptr<Element>> children_;
};

class GroupBox final : public Container
{
  public:
    explicit GroupBox(std::string title = {});
    void set_title(std::string title);
    void set_colors(uint16_t foreground, uint16_t background, uint16_t border);
    void draw(Renderer &renderer) override;

  private:
    std::string title_;
    uint16_t foreground_{0x0000};
    uint16_t background_{0xFFFF};
    uint16_t border_{0x0000};
};

class Label final : public Element
{
  public:
    explicit Label(std::string text = {});
    void set_text(std::string text);
    const std::string &text() const;
    void set_colors(uint16_t foreground, uint16_t background);
    void draw(Renderer &renderer) override;

  private:
    std::string text_;
    uint16_t foreground_{0x0000};
    uint16_t background_{0xFFFF};
};

class Checkbox final : public Element
{
  public:
    using Callback = std::function<void(bool)>;

    explicit Checkbox(std::string text = {});
    void set_text(std::string text);
    const std::string &text() const;
    void set_checked(bool checked);
    bool is_checked() const;
    void set_on_change(Callback callback);
    void draw(Renderer &renderer) override;
    void on_touch(int x, int y) override;
    void on_touch_end(int x, int y) override;

  private:
    std::string text_;
    Callback on_change_;
    bool checked_{false};
    bool pressed_{false};
    uint16_t foreground_{0x0000};
    uint16_t background_{0xFFFF};
};

class Button final : public Element
{
  public:
    using Callback = std::function<void()>;

    explicit Button(std::string text = {});
    void set_text(std::string text);
    void set_on_click(Callback callback);
    bool is_pressed() const;
    void unmount() override;
    void draw(Renderer &renderer) override;
    void on_touch(int x, int y) override;
    void on_touch_end(int x, int y) override;

  private:
    std::string text_;
    Callback on_click_;
    bool pressed_{false};
    uint16_t background_{0xC618};
};

class TouchDispatcher
{
  public:
    void update(Element &root, bool is_down, int x, int y);
    void poll(Element &root);

  private:
    bool was_down_{false};
    int last_x_{0};
    int last_y_{0};
};

} // namespace ui
