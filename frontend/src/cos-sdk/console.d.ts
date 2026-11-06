/**
 * Device console input. This is not the JavaScript `console` object.
 * Use the global `log` function to write a message.
 */

declare global {
  /**
   * Line editor on the COS console.
   *
   * `read_line` must be started on the console input task: a script started
   * with `berry run`, or an application's `setup`. While a prompt is active it
   * owns keyboard input, including when a UI is visible. A UI callback can
   * cancel the prompt and cannot start a second one.
   */
  namespace console {
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
     * Cancel the pending prompt. Its callback then receives `null`.
     *
     * @returns `true` when cancellation is accepted, `false` when no prompt is
     * waiting, or an ESP-IDF error name.
     */
    function cancel_input(): true | false | string;
  }
}

export {};
