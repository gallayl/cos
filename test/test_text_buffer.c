#include "unity.h"

#include "text_buffer.h"

#include <stdio.h>
#include <string.h>

static text_buffer_t buf;

void setUp(void)
{
    text_buffer_init(&buf, 20, 5);
}

void tearDown(void)
{
}

/* ── Initialization ────────────────────────────────────────── */

static void test_init_dimensions(void)
{
    TEST_ASSERT_EQUAL(20, buf.cols);
    TEST_ASSERT_EQUAL(5, buf.rows);
    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(0, buf.cursor_col);
    TEST_ASSERT_EQUAL(TEXT_BUF_DEFAULT_FG, buf.current_fg);
}

static void test_init_clamps_dimensions(void)
{
    text_buffer_t b;
    text_buffer_init(&b, 999, 999);
    TEST_ASSERT_EQUAL(TEXT_BUF_MAX_COLS, b.cols);
    TEST_ASSERT_EQUAL(TEXT_BUF_MAX_ROWS, b.rows);
}

static void test_init_cells_are_spaces(void)
{
    for (int r = 0; r < buf.rows; r++)
    {
        for (int c = 0; c < buf.cols; c++)
        {
            TEST_ASSERT_EQUAL(' ', buf.cells[r][c].ch);
            TEST_ASSERT_EQUAL(TEXT_BUF_DEFAULT_FG, buf.cells[r][c].fg);
        }
    }
}

static void test_init_all_dirty(void)
{
    TEST_ASSERT_TRUE(text_buffer_has_dirty(&buf));
}

/* ── Basic character writing ───────────────────────────────── */

static void test_write_single_char(void)
{
    text_buffer_write(&buf, "A", 1);
    TEST_ASSERT_EQUAL('A', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(1, buf.cursor_col);
}

static void test_write_string(void)
{
    text_buffer_write(&buf, "Hello", 5);
    TEST_ASSERT_EQUAL('H', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL('e', buf.cells[0][1].ch);
    TEST_ASSERT_EQUAL('l', buf.cells[0][2].ch);
    TEST_ASSERT_EQUAL('l', buf.cells[0][3].ch);
    TEST_ASSERT_EQUAL('o', buf.cells[0][4].ch);
    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(5, buf.cursor_col);
}

/* ── Control characters ────────────────────────────────────── */

static void test_newline(void)
{
    text_buffer_write(&buf, "AB\nCD", 5);
    TEST_ASSERT_EQUAL('A', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL('B', buf.cells[0][1].ch);
    TEST_ASSERT_EQUAL('C', buf.cells[1][0].ch);
    TEST_ASSERT_EQUAL('D', buf.cells[1][1].ch);
    TEST_ASSERT_EQUAL(1, buf.cursor_row);
    TEST_ASSERT_EQUAL(2, buf.cursor_col);
}

static void test_carriage_return(void)
{
    text_buffer_write(&buf, "ABCDE\rXY", 8);
    TEST_ASSERT_EQUAL('X', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL('Y', buf.cells[0][1].ch);
    TEST_ASSERT_EQUAL('C', buf.cells[0][2].ch);
    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(2, buf.cursor_col);
}

static void test_backspace(void)
{
    text_buffer_write(&buf, "ABC\b", 4);
    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(2, buf.cursor_col);
}

static void test_backspace_at_col_zero(void)
{
    text_buffer_write(&buf, "\b", 1);
    TEST_ASSERT_EQUAL(0, buf.cursor_col);
}

static void test_tab(void)
{
    text_buffer_write(&buf, "AB\t", 3);
    TEST_ASSERT_EQUAL(8, buf.cursor_col);
}

static void test_tab_already_at_stop(void)
{
    text_buffer_write(&buf, "12345678\t", 9);
    TEST_ASSERT_EQUAL(16, buf.cursor_col);
}

static void test_bell_ignored(void)
{
    text_buffer_write(&buf, "A\x07" "B", 3);
    TEST_ASSERT_EQUAL('A', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL('B', buf.cells[0][1].ch);
    TEST_ASSERT_EQUAL(2, buf.cursor_col);
}

/* ── Line wrapping ─────────────────────────────────────────── */

static void test_wrap_at_end_of_line(void)
{
    text_buffer_init(&buf, 5, 3);
    text_buffer_write(&buf, "ABCDE", 5);
    TEST_ASSERT_EQUAL('E', buf.cells[0][4].ch);
    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(4, buf.cursor_col);
    TEST_ASSERT_TRUE(buf.pending_wrap);
}

static void test_wrap_continues_on_next_row(void)
{
    text_buffer_init(&buf, 5, 3);
    text_buffer_write(&buf, "ABCDEF", 6);
    TEST_ASSERT_EQUAL('F', buf.cells[1][0].ch);
    TEST_ASSERT_EQUAL(1, buf.cursor_row);
    TEST_ASSERT_EQUAL(1, buf.cursor_col);
}

/* ── Scrolling ─────────────────────────────────────────────── */

static void test_scroll_on_screen_full(void)
{
    /* 5 cols, 3 rows. With deferred wrap: "AAAAA" fills row 0, pending_wrap set.
       \n clears wrap and moves to row 1. "BBBBB" fills row 1, \n to row 2.
       "CCCCC" fills row 2, \n scrolls: row 0=BBBBB, row 1=CCCCC, row 2=empty. */
    text_buffer_init(&buf, 5, 3);
    text_buffer_write(&buf, "AAAAA\n", 6);
    text_buffer_write(&buf, "BBBBB\n", 6);
    text_buffer_write(&buf, "CCCCC\n", 6);

    TEST_ASSERT_EQUAL('B', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL('C', buf.cells[1][0].ch);
    TEST_ASSERT_EQUAL(' ', buf.cells[2][0].ch);
    TEST_ASSERT_EQUAL(2, buf.cursor_row);
    TEST_ASSERT_EQUAL(0, buf.cursor_col);
}

static void test_scroll_multiple_times(void)
{
    text_buffer_init(&buf, 5, 2);
    text_buffer_write(&buf, "AA\nBB\nCC\nDD", 11);

    TEST_ASSERT_EQUAL('C', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL('D', buf.cells[1][0].ch);
    TEST_ASSERT_EQUAL(1, buf.cursor_row);
    TEST_ASSERT_EQUAL(2, buf.cursor_col);
}

/* ── Clear ─────────────────────────────────────────────────── */

static void test_clear(void)
{
    text_buffer_write(&buf, "Hello\nWorld", 11);
    text_buffer_clear(&buf);

    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(0, buf.cursor_col);
    TEST_ASSERT_EQUAL(' ', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL(TEXT_BUF_DEFAULT_FG, buf.current_fg);
}

/* ── CSI cursor movement ───────────────────────────────────── */

static void test_csi_cursor_forward(void)
{
    text_buffer_write(&buf, "\033[5C", 4);
    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(5, buf.cursor_col);
}

static void test_csi_cursor_backward(void)
{
    text_buffer_write(&buf, "ABCDE", 5);
    text_buffer_write(&buf, "\033[3D", 4);
    TEST_ASSERT_EQUAL(2, buf.cursor_col);
}

static void test_csi_cursor_up(void)
{
    text_buffer_write(&buf, "\n\n\n", 3);
    TEST_ASSERT_EQUAL(3, buf.cursor_row);
    text_buffer_write(&buf, "\033[2A", 4);
    TEST_ASSERT_EQUAL(1, buf.cursor_row);
}

static void test_csi_cursor_down(void)
{
    text_buffer_write(&buf, "\033[3B", 4);
    TEST_ASSERT_EQUAL(3, buf.cursor_row);
}

static void test_csi_cursor_home(void)
{
    text_buffer_write(&buf, "Hello\nWorld", 11);
    text_buffer_write(&buf, "\033[H", 3);
    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(0, buf.cursor_col);
}

static void test_csi_cursor_clamps_to_bounds(void)
{
    text_buffer_write(&buf, "\033[999C", 6);
    TEST_ASSERT_EQUAL(buf.cols - 1, buf.cursor_col);

    text_buffer_write(&buf, "\033[999D", 6);
    TEST_ASSERT_EQUAL(0, buf.cursor_col);

    text_buffer_write(&buf, "\033[999B", 6);
    TEST_ASSERT_EQUAL(buf.rows - 1, buf.cursor_row);

    text_buffer_write(&buf, "\033[999A", 6);
    TEST_ASSERT_EQUAL(0, buf.cursor_row);
}

static void test_csi_default_param_is_one(void)
{
    text_buffer_write(&buf, "ABCDE", 5);
    text_buffer_write(&buf, "\033[D", 3);
    TEST_ASSERT_EQUAL(4, buf.cursor_col);

    text_buffer_write(&buf, "\033[C", 3);
    TEST_ASSERT_EQUAL(5, buf.cursor_col);
}

/* ── CSI erase ─────────────────────────────────────────────── */

static void test_csi_erase_to_eol(void)
{
    text_buffer_write(&buf, "ABCDEFGHIJ", 10);
    text_buffer_write(&buf, "\r", 1);
    text_buffer_write(&buf, "\033[3C", 4);
    text_buffer_write(&buf, "\033[K", 3);

    TEST_ASSERT_EQUAL('A', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL('B', buf.cells[0][1].ch);
    TEST_ASSERT_EQUAL('C', buf.cells[0][2].ch);
    TEST_ASSERT_EQUAL(' ', buf.cells[0][3].ch);
    TEST_ASSERT_EQUAL(' ', buf.cells[0][4].ch);
}

static void test_csi_erase_display(void)
{
    text_buffer_write(&buf, "Hello\nWorld", 11);
    text_buffer_write(&buf, "\033[2J", 4);

    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(0, buf.cursor_col);
    TEST_ASSERT_EQUAL(' ', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL(' ', buf.cells[1][0].ch);
}

/* ── SGR colors ────────────────────────────────────────────── */

static void test_sgr_red(void)
{
    text_buffer_write(&buf, "\033[0;31mE", 8);
    TEST_ASSERT_EQUAL('E', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL(TEXT_BUF_COLOR_RED, buf.cells[0][0].fg);
}

static void test_sgr_green(void)
{
    text_buffer_write(&buf, "\033[0;32mI", 8);
    TEST_ASSERT_EQUAL(TEXT_BUF_COLOR_GREEN, buf.cells[0][0].fg);
}

static void test_sgr_yellow(void)
{
    text_buffer_write(&buf, "\033[0;33mW", 8);
    TEST_ASSERT_EQUAL(TEXT_BUF_COLOR_YELLOW, buf.cells[0][0].fg);
}

static void test_sgr_reset(void)
{
    text_buffer_write(&buf, "\033[0;31mE\033[0m normal", 19);
    TEST_ASSERT_EQUAL(TEXT_BUF_COLOR_RED, buf.cells[0][0].fg);
    TEST_ASSERT_EQUAL(TEXT_BUF_DEFAULT_FG, buf.cells[0][2].fg);
}

static void test_sgr_reset_bare_m(void)
{
    text_buffer_write(&buf, "\033[31mR\033[mN", 11);
    TEST_ASSERT_EQUAL(TEXT_BUF_COLOR_RED, buf.cells[0][0].fg);
    TEST_ASSERT_EQUAL(TEXT_BUF_DEFAULT_FG, buf.cells[0][1].fg);
}

static void test_sgr_all_basic_colors(void)
{
    const uint16_t expected[] = {
        TEXT_BUF_COLOR_BLACK,
        TEXT_BUF_COLOR_RED,
        TEXT_BUF_COLOR_GREEN,
        TEXT_BUF_COLOR_YELLOW,
        TEXT_BUF_COLOR_BLUE,
        TEXT_BUF_COLOR_MAGENTA,
        TEXT_BUF_COLOR_CYAN,
        TEXT_BUF_COLOR_WHITE,
    };
    char seq[16];
    for (int i = 0; i < 8; i++)
    {
        text_buffer_clear(&buf);
        int len = snprintf(seq, sizeof(seq), "\033[%dmX", 30 + i);
        text_buffer_write(&buf, seq, (size_t)len);
        TEST_ASSERT_EQUAL(expected[i], buf.cells[0][0].fg);
    }
}

static void test_sgr_bold_color_hint(void)
{
    text_buffer_write(&buf, "\033[1;32;49mH", 11);
    TEST_ASSERT_EQUAL(TEXT_BUF_COLOR_GREEN, buf.cells[0][0].fg);
}

static void test_sgr_unknown_param_ignored(void)
{
    text_buffer_write(&buf, "\033[99mA", 6);
    TEST_ASSERT_EQUAL('A', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL(TEXT_BUF_DEFAULT_FG, buf.cells[0][0].fg);
}

/* ── Unknown / malformed sequences ─────────────────────────── */

static void test_unknown_csi_ignored(void)
{
    text_buffer_write(&buf, "\033[?25hA", 7);
    TEST_ASSERT_EQUAL('A', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(1, buf.cursor_col);
}

static void test_bare_esc_not_followed_by_bracket(void)
{
    text_buffer_write(&buf, "\033)A", 3);
    TEST_ASSERT_EQUAL('A', buf.cells[0][0].ch);
}

static void test_incomplete_esc_at_end(void)
{
    text_buffer_write(&buf, "X\033", 2);
    TEST_ASSERT_EQUAL('X', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL(TB_STATE_ESC_SEEN, buf.state);

    text_buffer_write(&buf, "[1mY", 4);
    TEST_ASSERT_EQUAL('Y', buf.cells[0][1].ch);
    TEST_ASSERT_EQUAL(TB_STATE_NORMAL, buf.state);
}

/* ── Dirty tracking ────────────────────────────────────────── */

static void test_dirty_cleared_after_init(void)
{
    TEST_ASSERT_TRUE(text_buffer_has_dirty(&buf));

    for (int r = 0; r < buf.rows; r++)
    {
        for (int c = 0; c < buf.cols; c++)
        {
            buf.cells[r][c].dirty = false;
        }
    }
    TEST_ASSERT_FALSE(text_buffer_has_dirty(&buf));
}

static void test_write_sets_dirty(void)
{
    for (int r = 0; r < buf.rows; r++)
    {
        for (int c = 0; c < buf.cols; c++)
        {
            buf.cells[r][c].dirty = false;
        }
    }

    text_buffer_write(&buf, "A", 1);
    TEST_ASSERT_TRUE(buf.cells[0][0].dirty);
}

/* ── Combined sequences (realistic ESP-IDF log line) ───────── */

static void test_esp_log_info_line(void)
{
    /* "I (1234) main: Hello" = 20 chars in a 20-col buffer. */
    const char *line = "\033[0;32mI (1234) main: Hello\033[0m\n";
    text_buffer_write(&buf, line, strlen(line));

    TEST_ASSERT_EQUAL(TEXT_BUF_COLOR_GREEN, buf.cells[0][0].fg);
    TEST_ASSERT_EQUAL('I', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL(' ', buf.cells[0][1].ch);

    /* Deferred wrap: 20 chars fit in 20 cols with pending_wrap.
       The \n clears pending_wrap and moves to row 1. */
    TEST_ASSERT_EQUAL(1, buf.cursor_row);
    TEST_ASSERT_EQUAL(0, buf.cursor_col);
}

static void test_esp_log_error_line(void)
{
    const char *line = "\033[0;31mE (5678) wifi: fail\033[0m\n";
    text_buffer_write(&buf, line, strlen(line));

    TEST_ASSERT_EQUAL(TEXT_BUF_COLOR_RED, buf.cells[0][0].fg);
    TEST_ASSERT_EQUAL('E', buf.cells[0][0].ch);
}

static void test_linenoise_refresh_line(void)
{
    text_buffer_write(&buf, "COS> hello", 10);

    const char *refresh = "\r" "COS> hell" "\033[0K" "\r\033[9C";
    text_buffer_write(&buf, refresh, strlen(refresh));

    TEST_ASSERT_EQUAL('C', buf.cells[0][0].ch);
    TEST_ASSERT_EQUAL('O', buf.cells[0][1].ch);
    TEST_ASSERT_EQUAL('S', buf.cells[0][2].ch);
    TEST_ASSERT_EQUAL('>', buf.cells[0][3].ch);
    TEST_ASSERT_EQUAL(' ', buf.cells[0][4].ch);
    TEST_ASSERT_EQUAL('h', buf.cells[0][5].ch);
    TEST_ASSERT_EQUAL('e', buf.cells[0][6].ch);
    TEST_ASSERT_EQUAL('l', buf.cells[0][7].ch);
    TEST_ASSERT_EQUAL('l', buf.cells[0][8].ch);
    TEST_ASSERT_EQUAL(' ', buf.cells[0][9].ch);

    TEST_ASSERT_EQUAL(0, buf.cursor_row);
    TEST_ASSERT_EQUAL(9, buf.cursor_col);
}

int main(void)
{
    UNITY_BEGIN();

    /* Initialization */
    RUN_TEST(test_init_dimensions);
    RUN_TEST(test_init_clamps_dimensions);
    RUN_TEST(test_init_cells_are_spaces);
    RUN_TEST(test_init_all_dirty);

    /* Basic character writing */
    RUN_TEST(test_write_single_char);
    RUN_TEST(test_write_string);

    /* Control characters */
    RUN_TEST(test_newline);
    RUN_TEST(test_carriage_return);
    RUN_TEST(test_backspace);
    RUN_TEST(test_backspace_at_col_zero);
    RUN_TEST(test_tab);
    RUN_TEST(test_tab_already_at_stop);
    RUN_TEST(test_bell_ignored);

    /* Line wrapping */
    RUN_TEST(test_wrap_at_end_of_line);
    RUN_TEST(test_wrap_continues_on_next_row);

    /* Scrolling */
    RUN_TEST(test_scroll_on_screen_full);
    RUN_TEST(test_scroll_multiple_times);

    /* Clear */
    RUN_TEST(test_clear);

    /* CSI cursor movement */
    RUN_TEST(test_csi_cursor_forward);
    RUN_TEST(test_csi_cursor_backward);
    RUN_TEST(test_csi_cursor_up);
    RUN_TEST(test_csi_cursor_down);
    RUN_TEST(test_csi_cursor_home);
    RUN_TEST(test_csi_cursor_clamps_to_bounds);
    RUN_TEST(test_csi_default_param_is_one);

    /* CSI erase */
    RUN_TEST(test_csi_erase_to_eol);
    RUN_TEST(test_csi_erase_display);

    /* SGR colors */
    RUN_TEST(test_sgr_red);
    RUN_TEST(test_sgr_green);
    RUN_TEST(test_sgr_yellow);
    RUN_TEST(test_sgr_reset);
    RUN_TEST(test_sgr_reset_bare_m);
    RUN_TEST(test_sgr_all_basic_colors);
    RUN_TEST(test_sgr_bold_color_hint);
    RUN_TEST(test_sgr_unknown_param_ignored);

    /* Unknown / malformed sequences */
    RUN_TEST(test_unknown_csi_ignored);
    RUN_TEST(test_bare_esc_not_followed_by_bracket);
    RUN_TEST(test_incomplete_esc_at_end);

    /* Dirty tracking */
    RUN_TEST(test_dirty_cleared_after_init);
    RUN_TEST(test_write_sets_dirty);

    /* Combined sequences */
    RUN_TEST(test_esp_log_info_line);
    RUN_TEST(test_esp_log_error_line);
    RUN_TEST(test_linenoise_refresh_line);

    return UNITY_END();
}
