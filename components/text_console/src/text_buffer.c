#include "text_buffer.h"
#include <stdlib.h>
#include <string.h>

static const uint16_t s_ansi_color_table[8] = {
    TEXT_BUF_COLOR_BLACK, TEXT_BUF_COLOR_RED,     TEXT_BUF_COLOR_GREEN, TEXT_BUF_COLOR_YELLOW,
    TEXT_BUF_COLOR_BLUE,  TEXT_BUF_COLOR_MAGENTA, TEXT_BUF_COLOR_CYAN,  TEXT_BUF_COLOR_WHITE,
};

#define TEXT_HISTORY_DEFAULT_COLOR_INDEX 8

static uint8_t history_color_index(uint16_t foreground)
{
    for (uint8_t i = 0; i < sizeof(s_ansi_color_table) / sizeof(s_ansi_color_table[0]); i++)
    {
        if (s_ansi_color_table[i] == foreground)
        {
            return i;
        }
    }
    return TEXT_HISTORY_DEFAULT_COLOR_INDEX;
}

static uint16_t history_foreground(uint8_t index)
{
    return index < sizeof(s_ansi_color_table) / sizeof(s_ansi_color_table[0]) ? s_ansi_color_table[index]
                                                                              : TEXT_BUF_DEFAULT_FG;
}

static void clear_cell(text_cell_t *cell, uint16_t fg)
{
    cell->ch = ' ';
    cell->fg = fg;
    cell->dirty = true;
}

static void mark_grid_row_dirty(text_buffer_t *buf, int row)
{
    if (row < 0 || row >= buf->rows)
    {
        return;
    }

    for (int c = 0; c < buf->cols; c++)
    {
        buf->cells[row][c].dirty = true;
    }
    const int display_row = row + buf->viewport_offset;
    if (display_row < buf->rows)
    {
        buf->dirty_rows |= (1ULL << display_row);
    }
}

static void mark_all_rows_dirty(text_buffer_t *buf)
{
    for (int r = 0; r < buf->rows; r++)
    {
        for (int c = 0; c < buf->cols; c++)
        {
            buf->cells[r][c].dirty = true;
        }
    }
    buf->dirty_rows = (1ULL << buf->rows) - 1;
}

static void clear_row(text_buffer_t *buf, int row)
{
    for (int c = 0; c < buf->cols; c++)
    {
        clear_cell(&buf->cells[row][c], TEXT_BUF_DEFAULT_FG);
    }
    const int display_row = row + buf->viewport_offset;
    if (display_row < buf->rows)
    {
        buf->dirty_rows |= (1ULL << display_row);
    }
}

static void retain_scrolled_row(text_buffer_t *buf)
{
    if (buf->scrollback_disabled)
    {
        return;
    }

    if (buf->scrollback == NULL)
    {
        buf->scrollback = malloc(sizeof(*buf->scrollback) * TEXT_BUF_SCROLLBACK_ROWS);
        if (buf->scrollback == NULL)
        {
            buf->scrollback_disabled = true;
            buf->scrollback_allocation_failed = true;
            return;
        }
    }

    int index;
    if (buf->scrollback_count < TEXT_BUF_SCROLLBACK_ROWS)
    {
        index = (buf->scrollback_start + buf->scrollback_count) % TEXT_BUF_SCROLLBACK_ROWS;
        buf->scrollback_count++;
    }
    else
    {
        index = buf->scrollback_start;
        buf->scrollback_start = (buf->scrollback_start + 1) % TEXT_BUF_SCROLLBACK_ROWS;
    }

    for (int c = 0; c < buf->cols; c++)
    {
        buf->scrollback[index][c].ch = buf->cells[0][c].ch;
        buf->scrollback[index][c].fg_index = history_color_index(buf->cells[0][c].fg);
    }

    if (buf->viewport_offset > 0 && buf->viewport_offset < TEXT_BUF_SCROLLBACK_ROWS)
    {
        buf->viewport_offset++;
        if (buf->viewport_offset > buf->scrollback_count)
        {
            buf->viewport_offset = buf->scrollback_count;
        }
    }
}

static void scroll_up(text_buffer_t *buf)
{
    retain_scrolled_row(buf);
    for (int r = 1; r < buf->rows; r++)
    {
        memcpy(buf->cells[r - 1], buf->cells[r], sizeof(text_cell_t) * (size_t)buf->cols);
        for (int c = 0; c < buf->cols; c++)
        {
            buf->cells[r - 1][c].dirty = true;
        }
    }
    clear_row(buf, buf->rows - 1);
    mark_all_rows_dirty(buf);
}

static void next_line(text_buffer_t *buf)
{
    buf->cursor_col = 0;
    buf->cursor_row++;
    if (buf->cursor_row >= buf->rows)
    {
        scroll_up(buf);
        buf->cursor_row = buf->rows - 1;
    }
}

static void resolve_pending_wrap(text_buffer_t *buf)
{
    if (buf->pending_wrap)
    {
        buf->pending_wrap = false;
        next_line(buf);
    }
}

static void put_char(text_buffer_t *buf, char ch)
{
    resolve_pending_wrap(buf);

    text_cell_t *cell = &buf->cells[buf->cursor_row][buf->cursor_col];
    cell->ch = ch;
    cell->fg = buf->current_fg;
    cell->dirty = true;
    const int display_row = buf->cursor_row + buf->viewport_offset;
    if (display_row < buf->rows)
    {
        buf->dirty_rows |= (1ULL << display_row);
    }

    buf->cursor_col++;
    if (buf->cursor_col >= buf->cols)
    {
        buf->cursor_col = buf->cols - 1;
        buf->pending_wrap = true;
    }
}

static void handle_newline(text_buffer_t *buf)
{
    buf->pending_wrap = false;
    next_line(buf);
}

static void handle_carriage_return(text_buffer_t *buf)
{
    buf->pending_wrap = false;
    buf->cursor_col = 0;
}

static void handle_backspace(text_buffer_t *buf)
{
    if (buf->cursor_col > 0)
    {
        buf->cursor_col--;
    }
}

static void handle_tab(text_buffer_t *buf)
{
    int next_stop = (buf->cursor_col + 8) & ~7;
    if (next_stop > buf->cols)
    {
        next_stop = buf->cols;
    }
    while (buf->cursor_col < next_stop)
    {
        put_char(buf, ' ');
    }
}

static int clamp(int val, int min_val, int max_val)
{
    if (val < min_val)
    {
        return min_val;
    }
    if (val > max_val)
    {
        return max_val;
    }
    return val;
}

static void apply_sgr(text_buffer_t *buf)
{
    if (buf->csi_param_count == 0)
    {
        buf->current_fg = TEXT_BUF_DEFAULT_FG;
        return;
    }

    for (int i = 0; i < buf->csi_param_count; i++)
    {
        int p = buf->csi_params[i];
        if (p == 0)
        {
            buf->current_fg = TEXT_BUF_DEFAULT_FG;
        }
        else if (p >= 30 && p <= 37)
        {
            buf->current_fg = s_ansi_color_table[p - 30];
        }
        else if (p == 1)
        {
            /* Bold -- ignored for now, color already set */
        }
        else if (p == 49)
        {
            /* Default background -- ignored (always black) */
        }
    }
}

static void dispatch_csi(text_buffer_t *buf, char final_byte)
{
    buf->pending_wrap = false;
    int p0 = (buf->csi_param_count > 0) ? buf->csi_params[0] : 0;

    switch (final_byte)
    {
    case 'A': /* CUU - cursor up */
    {
        int n = (p0 > 0) ? p0 : 1;
        buf->cursor_row = clamp(buf->cursor_row - n, 0, buf->rows - 1);
        break;
    }
    case 'B': /* CUD - cursor down */
    {
        int n = (p0 > 0) ? p0 : 1;
        buf->cursor_row = clamp(buf->cursor_row + n, 0, buf->rows - 1);
        break;
    }
    case 'C': /* CUF - cursor forward */
    {
        int n = (p0 > 0) ? p0 : 1;
        buf->cursor_col = clamp(buf->cursor_col + n, 0, buf->cols - 1);
        break;
    }
    case 'D': /* CUB - cursor backward */
    {
        int n = (p0 > 0) ? p0 : 1;
        buf->cursor_col = clamp(buf->cursor_col - n, 0, buf->cols - 1);
        break;
    }
    case 'H': /* CUP - cursor home */
        buf->cursor_row = 0;
        buf->cursor_col = 0;
        break;
    case 'J': /* ED - erase in display */
        if (p0 == 2)
        {
            text_buffer_clear(buf);
        }
        break;
    case 'K': /* EL - erase in line */
        if (p0 == 0)
        {
            for (int c = buf->cursor_col; c < buf->cols; c++)
            {
                clear_cell(&buf->cells[buf->cursor_row][c], TEXT_BUF_DEFAULT_FG);
            }
            mark_grid_row_dirty(buf, buf->cursor_row);
        }
        break;
    case 'm': /* SGR - select graphic rendition */
        apply_sgr(buf);
        break;
    default:
        /* Unknown CSI final byte -- silently ignore */
        break;
    }
}

static void process_byte(text_buffer_t *buf, char ch)
{
    switch (buf->state)
    {
    case TB_STATE_NORMAL:
        if (ch == '\033')
        {
            buf->state = TB_STATE_ESC_SEEN;
        }
        else if (ch == '\n')
        {
            handle_newline(buf);
        }
        else if (ch == '\r')
        {
            handle_carriage_return(buf);
        }
        else if (ch == '\b')
        {
            handle_backspace(buf);
        }
        else if (ch == '\t')
        {
            handle_tab(buf);
        }
        else if (ch == '\x07')
        {
            /* BEL - ignore */
        }
        else if ((unsigned char)ch >= 0x20)
        {
            put_char(buf, ch);
        }
        break;

    case TB_STATE_ESC_SEEN:
        if (ch == '[')
        {
            buf->state = TB_STATE_CSI_PARAMS;
            buf->csi_param_count = 0;
            memset(buf->csi_params, 0, sizeof(buf->csi_params));
        }
        else
        {
            /* Not a CSI sequence -- discard and return to normal */
            buf->state = TB_STATE_NORMAL;
        }
        break;

    case TB_STATE_CSI_PARAMS:
        if (ch >= '0' && ch <= '9')
        {
            if (buf->csi_param_count == 0)
            {
                buf->csi_param_count = 1;
            }
            buf->csi_params[buf->csi_param_count - 1] = buf->csi_params[buf->csi_param_count - 1] * 10 + (ch - '0');
        }
        else if (ch == ';')
        {
            if (buf->csi_param_count < TEXT_BUF_CSI_MAX_PARAMS)
            {
                buf->csi_param_count++;
            }
        }
        else if (ch >= 0x40 && ch <= 0x7E)
        {
            /* Final byte -- dispatch and return to normal */
            dispatch_csi(buf, ch);
            buf->state = TB_STATE_NORMAL;
        }
        else if (ch >= 0x20 && ch <= 0x3F)
        {
            /* Intermediate/parameter bytes (e.g. '?', '>', ' ') -- consume silently */
        }
        else
        {
            /* Unexpected byte in CSI -- abort sequence */
            buf->state = TB_STATE_NORMAL;
        }
        break;
    }
}

void text_buffer_init(text_buffer_t *buf, int cols, int rows)
{
    memset(buf, 0, sizeof(*buf));
    buf->cols = clamp(cols, 1, TEXT_BUF_MAX_COLS);
    buf->rows = clamp(rows, 1, TEXT_BUF_MAX_ROWS);
    buf->current_fg = TEXT_BUF_DEFAULT_FG;
    buf->state = TB_STATE_NORMAL;

    for (int r = 0; r < buf->rows; r++)
    {
        for (int c = 0; c < buf->cols; c++)
        {
            buf->cells[r][c].ch = ' ';
            buf->cells[r][c].fg = TEXT_BUF_DEFAULT_FG;
            buf->cells[r][c].dirty = true;
        }
    }
    buf->dirty_rows = (1ULL << buf->rows) - 1;
}

void text_buffer_write(text_buffer_t *buf, const char *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        process_byte(buf, data[i]);
    }
}

void text_buffer_clear(text_buffer_t *buf)
{
    text_buffer_clear_scrollback(buf);
    for (int r = 0; r < buf->rows; r++)
    {
        clear_row(buf, r);
    }
    buf->cursor_row = 0;
    buf->cursor_col = 0;
    buf->pending_wrap = false;
    buf->current_fg = TEXT_BUF_DEFAULT_FG;
    buf->state = TB_STATE_NORMAL;
}

void text_buffer_clear_scrollback(text_buffer_t *buf)
{
    free(buf->scrollback);
    buf->scrollback = NULL;
    buf->scrollback_start = 0;
    buf->scrollback_count = 0;
    buf->viewport_offset = 0;
    buf->scrollback_disabled = false;
    buf->scrollback_allocation_failed = false;
    mark_all_rows_dirty(buf);
}

bool text_buffer_take_scrollback_allocation_failure(text_buffer_t *buf)
{
    const bool allocation_failed = buf->scrollback_allocation_failed;
    buf->scrollback_allocation_failed = false;
    return allocation_failed;
}

bool text_buffer_has_dirty(const text_buffer_t *buf)
{
    return buf->dirty_rows != 0;
}

bool text_buffer_scroll_view(text_buffer_t *buf, int rows)
{
    const int old_offset = buf->viewport_offset;
    if (rows > 0)
    {
        const int remaining = buf->scrollback_count - buf->viewport_offset;
        buf->viewport_offset += rows > remaining ? remaining : rows;
    }
    else if (rows < 0)
    {
        if (rows < -buf->viewport_offset)
        {
            buf->viewport_offset = 0;
        }
        else
        {
            buf->viewport_offset += rows;
        }
    }

    if (buf->viewport_offset == old_offset)
    {
        return false;
    }

    mark_all_rows_dirty(buf);
    return true;
}

bool text_buffer_get_visible_row(const text_buffer_t *buf, int row, char *chars, uint16_t *fg)
{
    if (row < 0 || row >= buf->rows || chars == NULL || fg == NULL)
    {
        return false;
    }

    const int history_row = buf->scrollback_count - buf->viewport_offset + row;
    if (history_row < buf->scrollback_count)
    {
        if (buf->scrollback == NULL)
        {
            return false;
        }
        const int index = (buf->scrollback_start + history_row) % TEXT_BUF_SCROLLBACK_ROWS;
        for (int c = 0; c < buf->cols; c++)
        {
            chars[c] = buf->scrollback[index][c].ch;
            fg[c] = history_foreground(buf->scrollback[index][c].fg_index);
        }
    }
    else
    {
        const int grid_row = history_row - buf->scrollback_count;
        if (grid_row < 0 || grid_row >= buf->rows)
        {
            return false;
        }
        for (int c = 0; c < buf->cols; c++)
        {
            chars[c] = buf->cells[grid_row][c].ch;
            fg[c] = buf->cells[grid_row][c].fg;
        }
    }
    return true;
}

void text_buffer_clear_row_dirty(text_buffer_t *buf, int row)
{
    if (row < 0 || row >= buf->rows)
    {
        return;
    }

    const int grid_row = row - buf->viewport_offset;
    if (grid_row >= 0 && grid_row < buf->rows)
    {
        for (int c = 0; c < buf->cols; c++)
        {
            buf->cells[grid_row][c].dirty = false;
        }
    }
    buf->dirty_rows &= ~(1ULL << row);
}

void text_buffer_resize(text_buffer_t *buf, int cols, int rows)
{
    buf->cols = clamp(cols, 1, TEXT_BUF_MAX_COLS);
    buf->rows = clamp(rows, 1, TEXT_BUF_MAX_ROWS);
    text_buffer_clear(buf);
}
