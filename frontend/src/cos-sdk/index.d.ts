/// <reference path="./language.d.ts" />
/// <reference path="./ui.d.ts" />
/// <reference path="./console.d.ts" />
/// <reference path="./filesystem.d.ts" />
/// <reference path="./network.d.ts" />
/// <reference path="./json.d.ts" />
/// <reference path="./berry-builtins.d.ts" />

/**
 * COS application entrypoints (R13). All runnable TS sources export a default
 * class implementing {@link CosApplication} or {@link CosUiApplication}.
 *
 * Detection is method-based (no path conventions):
 * - `setup(...)` present => UI app. May also declare `run(args)` for future
 *   CLI/R21 launch compatibility; under the current runtime, `berry run`
 *   starts the UI via `setup`.
 * - `run(args)` present and no `setup(...)` => console/command app.
 *
 * The compiler lowers to Berry in a way that remains compatible with current
 * firmware commands (`berry run`, `berry exec`, R9 command scripts, autoexec)
 * until R21 unifies the runtime contract.
 */

declare global {
  /**
   * Contract for COS applications. Console/command entrypoints implement
   * `run(args)`. UI entrypoints implement {@link CosUiApplication}.
   *
   * Declared as an ambient abstract class for TypeScript ergonomics and
   * `implements` checks; Berry output does not emit inheritance/implements.
   */
  abstract class CosApplication {
    /** Command/console entrypoint. `args` excludes the script path. */
    run?(args: string[]): number | void;
    /** Called when the application is torn down, when implemented. */
    teardown?(): void;
  }

  /**
   * UI application entrypoint. `berry run` constructs the class and calls
   * `setup` with the display size. `content` is reserved (device passes 0).
   */
  abstract class CosUiApplication extends CosApplication {
    abstract setup(content: unknown, width: number, height: number): void;
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
