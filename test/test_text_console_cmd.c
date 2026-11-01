#include "unity.h"

#include "mock_console.h"
#include "mock_text_console.h"
#include "text_console.h"

void setUp(void)
{
    mock_console_reset();
    mock_text_console_reset();
    text_console_register_commands();
}

void tearDown(void) {}

static void test_clear_command_clears_console(void)
{
    char *argv[] = {"clear"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("clear", 1, argv));
    TEST_ASSERT_EQUAL(1, mock_text_console_clear_count());
}

static void test_clear_command_rejects_arguments(void)
{
    char *argv[] = {"clear", "extra"};
    TEST_ASSERT_EQUAL(1, mock_console_run_cmd("clear", 2, argv));
    TEST_ASSERT_EQUAL(0, mock_text_console_clear_count());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clear_command_clears_console);
    RUN_TEST(test_clear_command_rejects_arguments);
    return UNITY_END();
}
