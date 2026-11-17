#include "unity.h"

#include "mock_console.h"
#include "shell.h"

void setUp(void)
{
    mock_console_reset();
    shell_register_services_commands();
}

void tearDown(void) {}

static void test_services_reports_core_services(void)
{
    char *argv[] = {"services"};
    TEST_ASSERT_EQUAL(0, mock_console_run_cmd("services", 1, argv));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_services_reports_core_services);
    return UNITY_END();
}
