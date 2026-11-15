/*
 * Migrated esp_littlefs Unity tests (extra subset) run under
 * COS integration_test structure.
 */

#include "unity.h"

void app_main(void)
{
    printf("Running esp_littlefs_extra tests\n");
    unity_run_all_tests();
}
