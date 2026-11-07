/**
 * Device console. This is not the JavaScript `console` object.
 * Use the global `log` function to write a device log.
 *
 * The screen is a character grid. `x` is the column and `y` is the row, both
 * 0-based, with the origin at the top left. `write` streams from the cursor.
 * `write_at` draws on one row and stops at the right edge.
 */

declare global {
  /**
   * Character console on the COS display.
   *
   * `read_line`, `read_key`, and `read_char` must be started on the console
   * input task: a script started with `berry run`, or an application's `setup`.
   * Only one of them can wait at a time. While it is waiting it owns keyboard
   * input, including when a UI is visible. A UI callback can cancel it and
   * cannot start a second read.
   */
  namespace console {
    /** Foreground color. The console background stays black. */
    type Color = "default" | "black" | "red" | "green" | "yellow" | "blue" | "magenta" | "cyan" | "white";

    /**
     * Names returned by {@link read_key} for keys that are not a single character.
     * A printable key is returned as that one character, such as `"y"`.
     */
    type NamedKey =
      | "up"
      | "down"
      | "left"
      | "right"
      | "enter"
      | "escape"
      | "backspace"
      | "tab"
      | "home"
      | "end"
      | "insert"
      | "delete"
      | "pageup"
      | "pagedown"
      | "f1"
      | "f2"
      | "f3"
      | "f4"
      | "f5"
      | "f6"
      | "f7"
      | "f8"
      | "f9"
      | "f10"
      | "f11"
      | "f12";

    /** Integer scale of the 6x8 cell. `1` is 6x8, `2` is 12x16, and `3` is 18x24 pixels. */
    type FontSize = 1 | 2 | 3;

    /** Active grid size, in character cells. */
    type Size = { cols: number; rows: number };

    /** Cursor position. `x` is the column and `y` is the row. */
    type Cursor = { x: number; y: number };

    /**
     * Start a non-blocking prompt.
     *
     * @param prompt Text shown before the input.
     * @param callback Receives the entered line, or `null` after Ctrl+C or
     * {@link cancel_input}.
     * @returns `true` when the prompt starts. Otherwise an ESP-IDF error name,
     * such as `ESP_ERR_INVALID_STATE` when a prompt is already waiting.
     */
    function read_line(prompt: string, callback: (line: string | null) => void): true | string;

    /**
     * Wait for the next key, including arrows. The key is not echoed.
     *
     * @param callback Receives a {@link NamedKey}, a one-character string, or
     * `null` after Ctrl+C or {@link cancel_input}.
     * @returns `true` when the read starts, or an ESP-IDF error name.
     */
    function read_key(callback: (key: NamedKey | string | null) => void): true | string;

    /**
     * Wait for the next printable character, such as `y` or `n`.
     * Arrows, Enter, and other named keys are ignored. The character is not echoed.
     *
     * @param callback Receives the character, or `null` after Ctrl+C or
     * {@link cancel_input}.
     * @returns `true` when the read starts, or an ESP-IDF error name.
     */
    function read_char(callback: (character: string | null) => void): true | string;

    /**
     * Cancel the pending `read_line`, `read_key`, or `read_char`.
     * Its callback then receives `null`.
     *
     * @returns `true` when cancellation is accepted, `false` when no read is
     * waiting, or an ESP-IDF error name.
     */
    function cancel_input(): true | false | string;

    /**
     * Move the cursor to a cell.
     *
     * @param x Column, starting at 0.
     * @param y Row, starting at 0.
     * @returns `true`, or an ESP-IDF error name when the cell is outside the grid.
     */
    function move(x: number, y: number): true | string;

    /**
     * Read the cursor.
     * @returns The cell, or an ESP-IDF error name.
     */
    function cursor(): Cursor | string;

    /**
     * Read the grid size.
     * @returns Columns and rows, or an ESP-IDF error name.
     */
    function size(): Size | string;

    /**
     * Write at the cursor and advance it. This does not add a newline.
     * Newline, carriage return, tab, and backspace follow the terminal.
     * A color applies only to this string. Strings longer than 1024 bytes are rejected.
     *
     * @returns `true`, or an ESP-IDF error name.
     */
    function write(text: string, color?: Color): true | string;

    /**
     * Draw `text` at a cell without wrapping onto the next row.
     * Characters past the right edge are dropped. The cursor stays on that row.
     *
     * @returns `true`, or an ESP-IDF error name when the cell is outside the grid.
     */
    function write_at(x: number, y: number, text: string, color?: Color): true | string;

    /**
     * Clear the grid and move the cursor to the top left.
     * @returns `true`, or an ESP-IDF error name.
     */
    function clear(): true | string;

    /**
     * Change the console cell size and clear the grid.
     * The column and row counts change with the scale.
     *
     * @returns `true`, or an ESP-IDF error name.
     */
    function set_font_size(size: FontSize): true | string;

    /**
     * Read the active cell scale.
     * @returns `1`, `2`, or `3`, or an ESP-IDF error name.
     */
    function font_size(): FontSize | string;
  }
}

export {};
