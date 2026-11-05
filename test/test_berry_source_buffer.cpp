#include "unity.h"

#include "berry_source_buffer.h"

#include <cstdlib>
#include <cstring>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

static void test_buffer_is_available_during_compilation_and_released_after_success(void)
{
    BerrySourceBuffer source(static_cast<char *>(std::malloc(5)), &std::free);
    TEST_ASSERT_NOT_NULL(source.get());
    std::memcpy(source.get(), "test", 5);

    bool source_available_during_compile = false;
    const int result = compile_and_release_source(source,
                                                  [&source_available_during_compile](const char *buffer)
                                                  {
                                                      source_available_during_compile =
                                                          buffer != nullptr && std::strcmp(buffer, "test") == 0;
                                                      return 0;
                                                  });

    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_TRUE(source_available_during_compile);
    TEST_ASSERT_NULL(source.get());
}

static void test_buffer_is_released_after_compilation_failure(void)
{
    BerrySourceBuffer source(static_cast<char *>(std::malloc(5)), &std::free);
    TEST_ASSERT_NOT_NULL(source.get());
    std::memcpy(source.get(), "test", 5);

    bool source_available_during_compile = false;
    const int result = compile_and_release_source(source,
                                                  [&source_available_during_compile](const char *buffer)
                                                  {
                                                      source_available_during_compile =
                                                          buffer != nullptr && std::strcmp(buffer, "test") == 0;
                                                      return -1;
                                                  });

    TEST_ASSERT_EQUAL(-1, result);
    TEST_ASSERT_TRUE(source_available_during_compile);
    TEST_ASSERT_NULL(source.get());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_buffer_is_available_during_compilation_and_released_after_success);
    RUN_TEST(test_buffer_is_released_after_compilation_failure);
    return UNITY_END();
}
