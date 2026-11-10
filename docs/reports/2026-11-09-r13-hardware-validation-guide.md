# R13 Hardware Validation Guide — Memory-conscious compiler rules / unified app entrypoints

**Date:** 2026-11-09  
**Status:** ✅ **Hardware validation passed (2026-11-09, user report).** R13-HW1–HW7 passed. Detailed serial logs were not attached in chat; this record captures the user-confirmed pass and the expected checks.

## Scope

R13 (scope A, confirmed) standardizes COS app entrypoints in TypeScript and enforces always-on compiler rules:

- `CosApplication` (console/command): `run(args): number | void`
- `CosUiApplication extends CosApplication` (UI): `setup(content,width,height): void` (+ optional `run(args)` for R21 compatibility)
- Entrypoint kind is **method-based**: `setup` => UI; `run` only => console; `setup`+`run` => UI kind.
- All `storage/ts/**` sources migrated; `storage/flash_data/**/*.be` regenerated.
- Firmware hook: `components/berry_lang/src/berry_runtime.cpp` sets `_cos_cmd_exec` true/false around `execute_command_source` chunk execution so console bridge auto-run (`berry run`/autoexec path) does not double-run under `berry exec`.

## Prerequisites

1. Build and flash firmware containing the `berry_runtime.cpp` `_cos_cmd_exec` change.
2. Deploy regenerated scripts/apps to the device filesystem as you normally do (`/flash/...` and/or `/sdcard/...`). At minimum update:
   - `/flash/autoexec.be`
   - `/flash/apps/breakout.be`
   - `/flash/apps/tap_dash.be`
   - `/flash/scripts/*.be` (especially `arg_test.be`, `ping.be`, `clear.be`, `hello.be`, `prompt.be`)
3. Reboot after deploying `autoexec.be`.

## Validation checklist

| ID | Case | Steps | Pass criteria | Result | Notes |
|---|---|---|---|---|---|
| R13-HW1 | Autoexec boot (console bridge via `berry run` path) | Reboot device with regenerated `/flash/autoexec.be`. Observe startup. | Console clears/runs startup behavior once (same intent as before). No crash, no double execution, no "script must define main(args)" / unexpected Berry error in logs. | ✅ Pass (user, 2026-11-09) | Autoexec loaded via `berry_runtime_run_file`; console bridge auto-runs `main([])` only when `_cos_cmd_exec != true`. |
| R13-HW2 | UI app via `berry run` | `berry run /flash/apps/breakout.be` (or tap_dash). Interact briefly, then `berry close` / app exit. Repeat once. | App launches normally, `setup` runs, UI responsive, teardown/close returns to console cleanly. No regression vs pre-R13 behavior. | ✅ Pass (user, 2026-11-09) | UI lowering returns class for `execute_app_source`; guarded `main(args)` for exec misuse. |
| R13-HW3 | Console script via `berry exec` | `berry exec /flash/scripts/arg_test.be "argument 1" "asd asd asd argument 2"` | Prints args as before (`1: argument 1`, `2: asd asd asd argument 2`), exit status 0. Output appears once (no double-run). | ✅ Pass (user, 2026-11-09) | Exercises `execute_command_source` + `_cos_cmd_exec` guard. |
| R13-HW4 | Console script via `berry run` (compat) | `berry run /flash/scripts/hello.be` (or another no-arg console script). | Script executes its command behavior (hello logs) and returns to shell/app state as expected for a non-UI script (`run_file` require_app=false). No UI app started, no double-run. | ✅ Pass (user, 2026-11-09) | Primary command-script path remains `berry exec`/R9 resolver. |
| R13-HW5 | R9-resolved command script | Configure `path`/CWD as needed; run a migrated script by name or explicit path (e.g. `ping.be` pilot path previously validated). | Command resolves and runs with args; status/errors behave as before. No double-run. | ✅ Pass (user, 2026-11-09) | Regression check for R9 + R13 lowering. |
| R13-HW6 | UI app misuse via `berry exec` | `berry exec /flash/apps/breakout.be` | Fails clearly/non-destructively. Expected: prints `UI app: launch with "berry run <path>" ...` and returns nonzero status (generated `main` guard). No UI corruption/crash. | ✅ Pass (user, 2026-11-09) | Confirms UI bridge `main(args)` guard. |
| R13-HW7 | Generated file smoke checks | Optionally run `files`, `font_size 1`, `clear`, `wifi` (read-only/no destructive args), `ping` (with network) as appropriate. | Commands behave as before migration. | ✅ Pass (user, 2026-11-09) | Spot-check class-wrapped console apps and method `this.*` usage. |

## Monaco/editor smoke test (no hardware required, recommended)

1. Open frontend editor, create/open a TypeScript file.
2. Type an invalid entrypoint (e.g. `log("x")` at top level, or `export default class X {}` with no `setup`/`run`).
3. Confirm debounced/live diagnostics and compile diagnostics show R13 errors with rule codes in output; Monaco squiggles/markers appear.
4. Use a valid UI entrypoint (`export default class App extends CosUiApplication { setup(...) {} }`) and console entrypoint (`export default class Cmd extends CosApplication { run(args) { return 0 } }`); confirm markers clear and compile succeeds.

## Evidence to record

- Firmware build/flash identifier or commit/workspace state.
- Files deployed (especially `autoexec.be`, one UI app, `arg_test.be`).
- Serial logs for R13-HW1, HW3, HW6.
- Any double-run, missing output, UI launch regression, or Berry errors.

## Follow-ups (if issues found)

- Double execution under `berry exec`: inspect `_cos_cmd_exec` handling and generated `if !(_cos_cmd_exec == true) main([]) end` guard.
- Autoexec regression: confirm deployed `autoexec.be` is regenerated class form and boot uses `berry_runtime_run_file`.
- `berry run` console-script behavior: if problematic, document as R21-adjacent; current primary contract is `berry exec`/R9 for command args.
- R21: replace runtime-compatible lowering bridges with unified `main(args)` + explicit UI launch API when implementing R21.
