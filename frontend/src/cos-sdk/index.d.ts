/// <reference path="./language.d.ts" />
/// <reference path="./ui.d.ts" />
/// <reference path="./console.d.ts" />
/// <reference path="./filesystem.d.ts" />
/// <reference path="./network.d.ts" />
/// <reference path="./json.d.ts" />
/// <reference path="./berry-builtins.d.ts" />

/**
 * Globals installed beside the `ui`, `console`, `fs`, and `net` modules.
 * `console.log` is not part of this SDK; write device logs with {@link log}.
 */

declare global {
  /**
   * Class returned to COS from `export default class`.
   *
   * The runtime instantiates it and calls `setup` with the display size.
   * `content` is reserved for a future container; the device currently passes `0`.
   * `teardown` runs on `berry close` and after {@link ui.exit}.
   * Either method may be omitted.
   */
  interface CosApp {
    setup(content: unknown, width: number, height: number): void;
    teardown?(): void;
  }

  /**
   * Write one message to the device log (`ESP_LOGI`, tag `berry`).
   * This does not print through the JavaScript console.
   */
  function log(message: string): void;

  /**
   * Run one registered console command, such as `"clear"`.
   *
   * @param command Command line, without a trailing newline.
   * @returns The command's integer status, or an ESP-IDF error name when the
   * command cannot be started. Printed command output is not returned.
   */
  function action(command: string): number | string;
}

export {};
