#include "keyboard_router.h"
#include "unity.h"

#include <string.h>

static bool s_console_owner;
static bool s_gui_owner;
static unsigned int s_gui_events;
static unsigned int s_shell_events;
static char s_shell_text[8];
static size_t s_shell_length;

static bool console_owns_input(void *context)
{
    (void)context;
    return s_console_owner;
}

static bool gui_handles_event(const keyboard_event_t *event, void *context)
{
    (void)event;
    (void)context;
    s_gui_events++;
    return s_gui_owner;
}

static void feed_shell(const char *text, size_t length, void *context)
{
    (void)context;
    s_shell_events++;
    s_shell_length = length < sizeof(s_shell_text) ? length : sizeof(s_shell_text);
    memcpy(s_shell_text, text, s_shell_length);
}

static const keyboard_router_t s_router = {
    .console_owns_input = console_owns_input,
    .gui_handles_event = gui_handles_event,
    .shell_input = feed_shell,
};

static keyboard_event_t key_event(bool pressed, const char *text, size_t text_len)
{
    keyboard_event_t event = {
        .keycode = 0x04,
        .pressed = pressed,
        .text_len = text_len,
    };
    if (text_len > 0)
    {
        memcpy(event.text, text, text_len);
    }
    return event;
}

static void reset_state(void)
{
    s_console_owner = false;
    s_gui_owner = false;
    s_gui_events = 0;
    s_shell_events = 0;
    memset(s_shell_text, 0, sizeof(s_shell_text));
    s_shell_length = 0;
}

void setUp(void)
{
    reset_state();
}

void tearDown(void) {}

static void test_console_prompt_owns_keyboard_before_gui(void)
{
    s_console_owner = true;
    s_gui_owner = true;
    const keyboard_event_t event = key_event(true, "a", 1);

    keyboard_route_event(&event, &s_router);

    TEST_ASSERT_EQUAL_UINT(0, s_gui_events);
    TEST_ASSERT_EQUAL_UINT(1, s_shell_events);
    TEST_ASSERT_EQUAL_UINT(1, s_shell_length);
    TEST_ASSERT_EQUAL_CHAR('a', s_shell_text[0]);
}

static void test_gui_owner_receives_press_and_release_without_shell_text(void)
{
    s_gui_owner = true;
    const keyboard_event_t press = key_event(true, "a", 1);
    const keyboard_event_t release = key_event(false, NULL, 0);

    keyboard_route_event(&press, &s_router);
    keyboard_route_event(&release, &s_router);

    TEST_ASSERT_EQUAL_UINT(2, s_gui_events);
    TEST_ASSERT_EQUAL_UINT(0, s_shell_events);
}

static void test_shell_receives_text_only_for_key_press_when_gui_does_not_consume(void)
{
    const keyboard_event_t press = key_event(true, "a", 1);
    const keyboard_event_t release = key_event(false, NULL, 0);

    keyboard_route_event(&press, &s_router);
    keyboard_route_event(&release, &s_router);

    TEST_ASSERT_EQUAL_UINT(2, s_gui_events);
    TEST_ASSERT_EQUAL_UINT(1, s_shell_events);
    TEST_ASSERT_EQUAL_UINT(1, s_shell_length);
    TEST_ASSERT_EQUAL_CHAR('a', s_shell_text[0]);
}

static void test_null_event_or_router_is_ignored(void)
{
    const keyboard_event_t event = key_event(true, "a", 1);

    keyboard_route_event(NULL, &s_router);
    keyboard_route_event(&event, NULL);

    TEST_ASSERT_EQUAL_UINT(0, s_gui_events);
    TEST_ASSERT_EQUAL_UINT(0, s_shell_events);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_console_prompt_owns_keyboard_before_gui);
    RUN_TEST(test_gui_owner_receives_press_and_release_without_shell_text);
    RUN_TEST(test_shell_receives_text_only_for_key_press_when_gui_does_not_consume);
    RUN_TEST(test_null_event_or_router_is_ignored);
    return UNITY_END();
}
