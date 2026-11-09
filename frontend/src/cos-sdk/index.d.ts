/// <reference path="./language.d.ts" />
/// <reference path="./ui.d.ts" />
/// <reference path="./console.d.ts" />
/// <reference path="./filesystem.d.ts" />
/// <reference path="./network.d.ts" />
/// <reference path="./json.d.ts" />
/// <reference path="./berry-builtins.d.ts" />

/**
 * COS application entrypoints (R13, unified by R21). All runnable TS sources
 * export a default class implementing {@link CosApplication} or
 * {@link CosUiApplication}.
 *
 * Detection is method-based (no path conventions):
 * - `setup(...)` present => UI app.
 * - `run(args)` present and no `setup(...)` => console/command app.
 * - Both present: UI entrypoint; `run(args)` may be used for CLI args.
 *
 * R21: `berry run <path> [args...]` executes `main(args)` for all entrypoints.
 * UI entrypoints lower to `main(args)` calling `ui.launch(AppClass)`. Console
 * entrypoints lower to `main(args)` calling `app.run(args)`. R9-resolved
 * command scripts use the same contract.
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
   * UI application entrypoint. `berry run` executes `main(args)`, which (as
   * lowered by the TS compiler) calls `ui.launch(AppClass)`. `ui.launch`
   * constructs the class if needed and calls `setup` with the display size.
   * `content` is reserved (device passes 0).
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
