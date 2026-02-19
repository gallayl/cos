#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TEXT_BUF_MAX_COLS 54
#define TEXT_BUF_MAX_ROWS 40

#define TEXT_BUF_DEFAULT_FG 0xFFFF
#define TEXT_BUF_COLOR_BLACK 0x0000
#define TEXT_BUF_COLOR_RED 0xF800
#define TEXT_BUF_COLOR_GREEN 0x07E0
#define TEXT_BUF_COLOR_YELLOW 0xFFE0
#define TEXT_BUF_COLOR_BLUE 0x001F
#define TEXT_BUF_COLOR_MAGENTA 0xF81F
#define TEXT_BUF_COLOR_CYAN 0x07FF
#define TEXT_BUF_COLOR_WHITE 0xFFFF

#define TEXT_BUF_CSI_MAX_PARAMS 4

#ifdef __cplusplus
extern "C"
{
#endif

    /** VT100 parser states. */
    typedef enum
    {
        TB_STATE_NORMAL,
        TB_STATE_ESC_SEEN,
        TB_STATE_CSI_PARAMS,
    } tb_parse_state_t;

    /** Cell in the character grid. */
    typedef struct
    {
        char ch;     /**< Character (space if empty). */
        uint16_t fg; /**< Foreground color (RGB565). */
        bool dirty;  /**< True if cell needs redraw. */
    } text_cell_t;

    /** Text buffer state. */
    typedef struct
    {
        text_cell_t cells[TEXT_BUF_MAX_ROWS][TEXT_BUF_MAX_COLS];
        int rows;                                /**< Active row count (<= MAX_ROWS). */
        int cols;                                /**< Active column count (<= MAX_COLS). */
        int cursor_row;                          /**< Current cursor row. */
        int cursor_col;                          /**< Current cursor column. */
        bool pending_wrap;                       /**< Deferred wrap: cursor at EOL, wrap on next printable char. */
        uint16_t current_fg;                     /**< Current foreground color for new chars. */
        tb_parse_state_t state;                  /**< VT100 parser state. */
        int csi_params[TEXT_BUF_CSI_MAX_PARAMS]; /**< CSI parameter accumulator. */
        int csi_param_count;                     /**< Number of CSI params parsed so far. */
    } text_buffer_t;

    /**
     * @brief Initialize a text buffer with the given dimensions.
     *
     * @param buf Buffer to initialize.
     * @param cols Number of columns (clamped to TEXT_BUF_MAX_COLS).
     * @param rows Number of rows (clamped to TEXT_BUF_MAX_ROWS).
     */
    void text_buffer_init(text_buffer_t *buf, int cols, int rows);

    /**
     * @brief Write data into the buffer, processing VT100 escape sequences.
     *
     * @param buf Buffer to write to.
     * @param data Input byte stream.
     * @param len Number of bytes in data.
     */
    void text_buffer_write(text_buffer_t *buf, const char *data, size_t len);

    /** Clear the entire buffer and reset cursor to (0, 0). */
    void text_buffer_clear(text_buffer_t *buf);

    /** Check if any cell is marked dirty. */
    bool text_buffer_has_dirty(const text_buffer_t *buf);

#ifdef __cplusplus
}
#endif
