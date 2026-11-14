/*
 * Migrated esp_littlefs Unity tests (from component test/) run under
 * COS integration_test structure.
 */

#include "unity.h"

void app_main(void)
{
    printf("Running esp_littlefs migrated tests\n");
    unity_run_all_tests();
}
