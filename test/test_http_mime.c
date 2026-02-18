#include "unity.h"
#include "http_server.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_html(void)
{
    TEST_ASSERT_EQUAL_STRING("text/html", http_mime_type("index.html"));
    TEST_ASSERT_EQUAL_STRING("text/html", http_mime_type("/path/to/page.htm"));
}

void test_js(void)
{
    TEST_ASSERT_EQUAL_STRING("application/javascript", http_mime_type("app.js"));
}

void test_css(void)
{
    TEST_ASSERT_EQUAL_STRING("text/css", http_mime_type("style.css"));
}

void test_json(void)
{
    TEST_ASSERT_EQUAL_STRING("application/json", http_mime_type("data.json"));
}

void test_png(void)
{
    TEST_ASSERT_EQUAL_STRING("image/png", http_mime_type("logo.png"));
}

void test_jpeg(void)
{
    TEST_ASSERT_EQUAL_STRING("image/jpeg", http_mime_type("photo.jpg"));
    TEST_ASSERT_EQUAL_STRING("image/jpeg", http_mime_type("photo.jpeg"));
}

void test_gif(void)
{
    TEST_ASSERT_EQUAL_STRING("image/gif", http_mime_type("anim.gif"));
}

void test_ico(void)
{
    TEST_ASSERT_EQUAL_STRING("image/x-icon", http_mime_type("favicon.ico"));
}

void test_svg(void)
{
    TEST_ASSERT_EQUAL_STRING("image/svg+xml", http_mime_type("icon.svg"));
}

void test_woff(void)
{
    TEST_ASSERT_EQUAL_STRING("font/woff", http_mime_type("font.woff"));
    TEST_ASSERT_EQUAL_STRING("font/woff2", http_mime_type("font.woff2"));
}

void test_unknown_extension(void)
{
    TEST_ASSERT_EQUAL_STRING("application/octet-stream", http_mime_type("file.xyz"));
    TEST_ASSERT_EQUAL_STRING("application/octet-stream", http_mime_type("file.bin"));
}

void test_no_extension(void)
{
    TEST_ASSERT_EQUAL_STRING("application/octet-stream", http_mime_type("Makefile"));
    TEST_ASSERT_EQUAL_STRING("application/octet-stream", http_mime_type("README"));
}

void test_empty_and_null(void)
{
    TEST_ASSERT_EQUAL_STRING("application/octet-stream", http_mime_type(""));
    TEST_ASSERT_EQUAL_STRING("application/octet-stream", http_mime_type(NULL));
}

void test_multiple_dots(void)
{
    TEST_ASSERT_EQUAL_STRING("application/javascript", http_mime_type("app.bundle.min.js"));
    TEST_ASSERT_EQUAL_STRING("text/html", http_mime_type("page.old.html"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_html);
    RUN_TEST(test_js);
    RUN_TEST(test_css);
    RUN_TEST(test_json);
    RUN_TEST(test_png);
    RUN_TEST(test_jpeg);
    RUN_TEST(test_gif);
    RUN_TEST(test_ico);
    RUN_TEST(test_svg);
    RUN_TEST(test_woff);
    RUN_TEST(test_unknown_extension);
    RUN_TEST(test_no_extension);
    RUN_TEST(test_empty_and_null);
    RUN_TEST(test_multiple_dots);
    return UNITY_END();
}
