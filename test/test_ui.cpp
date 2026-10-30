#include "unity.h"

#include "display.h"
#include "mock_display.h"
#include "ui.h"

#include <limits>
#include <memory>

namespace
{
class TestRenderer final : public ui::Renderer
{
  public:
    void fill_rect(const ui::Bounds &, uint16_t) override
    {
        ++fill_count;
    }

    void draw_hline(int, int, int, uint16_t) override
    {
        ++line_count;
    }

    void draw_vline(int, int, int, uint16_t) override
    {
        ++line_count;
    }

    void draw_text(const ui::Bounds &, const char *text, uint16_t, uint16_t) override
    {
        ++text_count;
        last_text = text;
    }

    int fill_count{0};
    int line_count{0};
    int text_count{0};
    std::string last_text;
};

void test_bounds_are_half_open_and_reject_empty_sizes()
{
    ui::Button button;
    button.set_bounds({10, 20, 30, 40});
    TEST_ASSERT_TRUE(button.contains(10, 20));
    TEST_ASSERT_TRUE(button.contains(39, 59));
    TEST_ASSERT_FALSE(button.contains(40, 59));
    TEST_ASSERT_FALSE(button.contains(39, 60));
    TEST_ASSERT_FALSE(button.contains(std::numeric_limits<int>::max(), 20));
    button.set_bounds({0, 0, 0, 10});
    TEST_ASSERT_FALSE(button.contains(0, 0));
}

void test_container_mounts_children_added_before_and_after_mount()
{
    ui::Container container;
    auto first = std::make_unique<ui::Button>("First");
    ui::Element *first_ptr = container.add_child(std::move(first));
    TEST_ASSERT_FALSE(first_ptr->is_mounted());

    container.mount();
    TEST_ASSERT_TRUE(first_ptr->is_mounted());

    auto second = std::make_unique<ui::Button>("Second");
    ui::Element *second_ptr = container.add_child(std::move(second));
    TEST_ASSERT_TRUE(second_ptr->is_mounted());

    container.unmount();
    TEST_ASSERT_FALSE(first_ptr->is_mounted());
    TEST_ASSERT_FALSE(second_ptr->is_mounted());
}

void test_touch_dispatcher_clicks_only_on_press_and_release_inside()
{
    ui::Container root;
    auto button = std::make_unique<ui::Button>("Run");
    button->set_bounds({5, 5, 20, 20});
    int click_count = 0;
    button->set_on_click([&click_count]() { ++click_count; });
    ui::Element *button_ptr = root.add_child(std::move(button));
    root.mount();

    ui::TouchDispatcher dispatcher;
    dispatcher.update(root, true, 10, 10);
    TEST_ASSERT_TRUE(static_cast<ui::Button *>(button_ptr)->is_pressed());
    dispatcher.update(root, false, 10, 10);
    TEST_ASSERT_EQUAL(1, click_count);
    TEST_ASSERT_FALSE(static_cast<ui::Button *>(button_ptr)->is_pressed());

    dispatcher.update(root, true, 10, 10);
    dispatcher.update(root, false, 30, 30);
    TEST_ASSERT_EQUAL(1, click_count);
    TEST_ASSERT_FALSE(static_cast<ui::Button *>(button_ptr)->is_pressed());
}

void test_touch_outside_does_not_press_button()
{
    ui::Button button;
    button.set_bounds({5, 5, 20, 20});
    button.mount();

    button.on_touch(30, 30);
    TEST_ASSERT_FALSE(button.is_pressed());
}

void test_unmount_cancels_active_button_press()
{
    ui::Button button;
    button.set_bounds({5, 5, 20, 20});
    button.mount();

    button.on_touch(10, 10);
    TEST_ASSERT_TRUE(button.is_pressed());
    button.unmount();
    TEST_ASSERT_FALSE(button.is_pressed());
}

void test_touch_polling_dispatches_display_events()
{
    ui::Button button;
    button.set_bounds({5, 5, 20, 20});
    int click_count = 0;
    button.set_on_click([&click_count]() { ++click_count; });
    button.mount();

    ui::TouchDispatcher dispatcher;
    mock_display_set_touch(true, 10, 10);
    int touch_x = 0;
    int touch_y = 0;
    TEST_ASSERT_TRUE(display_get_touch(&touch_x, &touch_y));
    TEST_ASSERT_EQUAL(10, touch_x);
    TEST_ASSERT_EQUAL(10, touch_y);
    dispatcher.poll(button);
    TEST_ASSERT_TRUE(button.is_pressed());

    mock_display_set_touch(false, 0, 0);
    dispatcher.poll(button);
    TEST_ASSERT_EQUAL(1, click_count);
}

void test_button_draws_raised_frame_and_label()
{
    ui::Button button("Launch");
    button.set_bounds({1, 2, 30, 12});
    button.mount();
    TestRenderer renderer;
    button.draw(renderer);

    TEST_ASSERT_EQUAL(1, renderer.fill_count);
    TEST_ASSERT_EQUAL(4, renderer.line_count);
    TEST_ASSERT_EQUAL(1, renderer.text_count);
    TEST_ASSERT_EQUAL_STRING("Launch", renderer.last_text.c_str());
}

void test_label_draws_only_when_mounted()
{
    ui::Label label("Status");
    label.set_bounds({0, 0, 40, 12});
    TestRenderer renderer;

    label.draw(renderer);
    TEST_ASSERT_EQUAL(0, renderer.text_count);

    label.mount();
    label.draw(renderer);
    TEST_ASSERT_EQUAL(1, renderer.text_count);
    TEST_ASSERT_EQUAL_STRING("Status", renderer.last_text.c_str());
}

void test_groupbox_draws_frame_title_and_children()
{
    ui::GroupBox group("Settings");
    group.set_bounds({2, 4, 80, 40});
    auto label = std::make_unique<ui::Label>("Brightness");
    label->set_bounds({6, 18, 60, 12});
    group.add_child(std::move(label));
    group.mount();
    TestRenderer renderer;

    group.draw(renderer);

    TEST_ASSERT_EQUAL(2, renderer.fill_count);
    TEST_ASSERT_GREATER_OR_EQUAL(4, renderer.line_count);
    TEST_ASSERT_EQUAL(2, renderer.text_count);
    TEST_ASSERT_EQUAL_STRING("Brightness", renderer.last_text.c_str());
}

void test_checkbox_toggles_and_notifies_after_touch_release()
{
    ui::Checkbox checkbox("Enabled");
    checkbox.set_bounds({4, 6, 80, 18});
    bool callback_called = false;
    bool callback_value = false;
    checkbox.set_on_change(
        [&](bool checked)
        {
            callback_called = true;
            callback_value = checked;
        });
    checkbox.mount();
    ui::TouchDispatcher dispatcher;

    dispatcher.update(checkbox, true, 8, 10);
    TEST_ASSERT_FALSE(checkbox.is_checked());
    TEST_ASSERT_FALSE(callback_called);
    dispatcher.update(checkbox, false, 8, 10);

    TEST_ASSERT_TRUE(checkbox.is_checked());
    TEST_ASSERT_TRUE(callback_called);
    TEST_ASSERT_TRUE(callback_value);
    dispatcher.update(checkbox, true, 100, 100);
    dispatcher.update(checkbox, false, 100, 100);
    TEST_ASSERT_TRUE(checkbox.is_checked());
}

void test_checkbox_draws_box_and_label()
{
    ui::Checkbox checkbox("Wi-Fi");
    checkbox.set_bounds({2, 3, 60, 16});
    checkbox.set_checked(true);
    checkbox.mount();
    TestRenderer renderer;

    checkbox.draw(renderer);

    TEST_ASSERT_EQUAL(1, renderer.fill_count);
    TEST_ASSERT_EQUAL(5, renderer.line_count);
    TEST_ASSERT_EQUAL(1, renderer.text_count);
    TEST_ASSERT_EQUAL_STRING("Wi-Fi", renderer.last_text.c_str());
}

void test_ui_start_mounts_screen_root_and_stop_unmounts_it()
{
    ui::Container &root = ui::app_root();
    TEST_ASSERT_TRUE(ui::start());
    TEST_ASSERT_TRUE(root.is_mounted());
    TEST_ASSERT_EQUAL(display_get_width(), root.bounds().width);
    TEST_ASSERT_EQUAL(display_get_height(), root.bounds().height);

    ui::stop();
    TEST_ASSERT_FALSE(root.is_mounted());
}
} // namespace

extern "C" void setUp(void)
{
    mock_display_reset();
}

extern "C" void tearDown(void) {}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_bounds_are_half_open_and_reject_empty_sizes);
    RUN_TEST(test_container_mounts_children_added_before_and_after_mount);
    RUN_TEST(test_touch_dispatcher_clicks_only_on_press_and_release_inside);
    RUN_TEST(test_touch_outside_does_not_press_button);
    RUN_TEST(test_unmount_cancels_active_button_press);
    RUN_TEST(test_touch_polling_dispatches_display_events);
    RUN_TEST(test_button_draws_raised_frame_and_label);
    RUN_TEST(test_label_draws_only_when_mounted);
    RUN_TEST(test_groupbox_draws_frame_title_and_children);
    RUN_TEST(test_checkbox_toggles_and_notifies_after_touch_release);
    RUN_TEST(test_checkbox_draws_box_and_label);
    RUN_TEST(test_ui_start_mounts_screen_root_and_stop_unmounts_it);
    return UNITY_END();
}
