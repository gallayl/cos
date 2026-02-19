#include "unity.h"
#include "bluetooth_eir.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* --- Valid EIR with Complete Local Name (type 0x09) --- */

void test_parse_complete_name(void)
{
    /* EIR record: length=6, type=0x09, data="Hello" */
    uint8_t eir[] = {0x06, 0x09, 'H', 'e', 'l', 'l', 'o'};
    char out[32] = {0};

    TEST_ASSERT_TRUE(bluetooth_parse_eir_name(eir, sizeof(eir), out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("Hello", out);
}

/* --- Valid EIR with Shortened Local Name (type 0x08) --- */

void test_parse_shortened_name(void)
{
    uint8_t eir[] = {0x04, 0x08, 'K', 'B', 'D'};
    char out[32] = {0};

    TEST_ASSERT_TRUE(bluetooth_parse_eir_name(eir, sizeof(eir), out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("KBD", out);
}

/* --- Name record not first in EIR --- */

void test_parse_name_after_other_records(void)
{
    /* First record: flags (type=0x01, length=2, data=0x06) */
    /* Second record: complete name (type=0x09, length=4, data="Foo") */
    uint8_t eir[] = {0x02, 0x01, 0x06, 0x04, 0x09, 'F', 'o', 'o'};
    char out[32] = {0};

    TEST_ASSERT_TRUE(bluetooth_parse_eir_name(eir, sizeof(eir), out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("Foo", out);
}

/* --- Name truncated to fit output buffer --- */

void test_parse_name_truncated(void)
{
    uint8_t eir[] = {0x08, 0x09, 'L', 'o', 'n', 'g', 'N', 'a', 'm'};
    char out[4] = {0}; /* only room for 3 chars + null */

    TEST_ASSERT_TRUE(bluetooth_parse_eir_name(eir, sizeof(eir), out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("Lon", out);
}

/* --- Empty EIR returns false --- */

void test_parse_empty_eir(void)
{
    char out[32] = "unchanged";
    TEST_ASSERT_FALSE(bluetooth_parse_eir_name(NULL, 0, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("unchanged", out);
}

void test_parse_null_eir(void)
{
    char out[32] = "unchanged";
    TEST_ASSERT_FALSE(bluetooth_parse_eir_name(NULL, 10, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("unchanged", out);
}

void test_parse_zero_length_eir(void)
{
    uint8_t eir[] = {0x01};
    char out[32] = "unchanged";
    TEST_ASSERT_FALSE(bluetooth_parse_eir_name(eir, 0, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("unchanged", out);
}

/* --- EIR with no name record returns false --- */

void test_parse_no_name_record(void)
{
    /* Only a flags record (type=0x01) */
    uint8_t eir[] = {0x02, 0x01, 0x06};
    char out[32] = "unchanged";

    TEST_ASSERT_FALSE(bluetooth_parse_eir_name(eir, sizeof(eir), out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("unchanged", out);
}

/* --- Malformed EIR: record length exceeds data --- */

void test_parse_malformed_length_too_large(void)
{
    /* Record claims length=0x10 but only 3 bytes of EIR data */
    uint8_t eir[] = {0x10, 0x09, 'X'};
    char out[32] = "unchanged";

    TEST_ASSERT_FALSE(bluetooth_parse_eir_name(eir, sizeof(eir), out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("unchanged", out);
}

/* --- EIR with zero-length record terminates parsing --- */

void test_parse_zero_length_record_terminates(void)
{
    /* Zero-length record followed by a valid name record -- should stop at zero */
    uint8_t eir[] = {0x00, 0x04, 0x09, 'F', 'o', 'o'};
    char out[32] = "unchanged";

    TEST_ASSERT_FALSE(bluetooth_parse_eir_name(eir, sizeof(eir), out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("unchanged", out);
}

/* --- Single-character name --- */

void test_parse_single_char_name(void)
{
    uint8_t eir[] = {0x02, 0x09, 'X'};
    char out[32] = {0};

    TEST_ASSERT_TRUE(bluetooth_parse_eir_name(eir, sizeof(eir), out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("X", out);
}

/* --- main --- */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_complete_name);
    RUN_TEST(test_parse_shortened_name);
    RUN_TEST(test_parse_name_after_other_records);
    RUN_TEST(test_parse_name_truncated);
    RUN_TEST(test_parse_empty_eir);
    RUN_TEST(test_parse_null_eir);
    RUN_TEST(test_parse_zero_length_eir);
    RUN_TEST(test_parse_no_name_record);
    RUN_TEST(test_parse_malformed_length_too_large);
    RUN_TEST(test_parse_zero_length_record_terminates);
    RUN_TEST(test_parse_single_char_name);

    return UNITY_END();
}
