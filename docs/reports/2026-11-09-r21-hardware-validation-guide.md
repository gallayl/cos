# R21 — Hardware Validation Guide: Unified `berry run` / `ui.launch`

**Date:** 2026-11-09  
**Plan item:** R21 (P2, M9) — Unify `berry run` entrypoint contract + explicit app launch  
**Status:** ✅ **Initial validation complete (2026-11-09, `main(args)` contract, HW1–HW8 OK).**  
**Follow-up (2026-11-10):** the `main(args)` bridge was removed and replaced by the `__cos_entry` / `run(args)` entrypoint contract (no `def main`). The contract summary and checklist below reflect the original 2026-11-09 validation; current contract details are in `README.md`, `components/berry_lang/include/berry_runtime.h`, and `docs/plan/consolidated-improvement-plan.md` (R21). **Hardware (user, 2026-11-10):** autoexec runs successfully with the new contract after runtime stack-fix rebuild/reflash (`action("clear")`, status 0). The full HW1–HW8 matrix was not formally rerun for this follow-up.

## Summary of R21 behavior (validated 2026-11-09, `main(args)` contract)

- `berry run <path> [args...]` is the unified command. It loads the script, executes top-level code, then calls `main(args)` where `args` is a Berry list of command arguments **excluding the script path**. `main(args)` returns an integer shell status (`nil` means `0`).
- Default execution is **non-intrusive**: it does not close/disturb an active UI app.
- UI apps launch explicitly from `main(args)` via `ui.launch(AppClassOrInstance)`. `ui.launch` performs app lifecycle (replaces/closes active app as specified), constructs the class if a class is passed, calls `setup(0, display_width, display_height)`, and starts the UI. It returns `0` on success and raises on failure.
- `berry exec` is **removed**. R9-resolved command scripts and `path`-resolved scripts use the same `main(args)` contract.
- TS authoring pattern is preserved: `export default class ... implements CosUiApplication` / `CosApplication`. The compiler lowers UI entrypoints to `main(args)` + `ui.launch(...)` and console entrypoints to `main(args)` bridging `app.run(args)`.
- Missing `main`, non-integer/non-nil return values, and runtime errors produce clear failures (nonzero/`ESP_FAIL`/`ESP_ERR_INVALID_ARG` as appropriate).

## Follow-up contract (2026-11-10, current)

`def main(args)` is no longer used. After top-level execution the runtime resolves:
- `__cos_entry` = UI class/instance with `setup` → launched by runtime (status `0` on success).
- `__cos_entry` = command instance with `run(args)` → `run(args)` invoked, `nil`→`0`, integer status required.
- no `__cos_entry` → global `run(args)` invoked as command entrypoint.

Compiler output: `var __cos_entry = <Class>` (UI) or `var __cos_entry = <Class>()` (console). See `README.md` / runtime header for the authoritative API. Hardware confirmation (user, 2026-11-10): autoexec OK after runtime `be_pcall` return-slot fix (`action("clear")`, status 0).

## Preconditions

- Flash firmware containing R21 runtime/shell/compiler-generated Berry files.
- Deploy updated `storage/flash_data` contents (especially `apps/*.be`, `scripts/*.be`, `autoexec.be`, `tests/**`) to `/flash` and/or `/sdcard` as needed.
- Wi-Fi/network only needed for network-related scripts (ping/fetch); core R21 checks are local.
- Capture serial logs for failures.

## Checklist

### R21-HW1 — Autoexec / console startup

1. Reboot with `/flash/autoexec.be` present (stock autoexec clears console).
2. **Expect:** no crash; console behavior matches autoexec script; shell becomes responsive.
3. If autoexec returns nonzero, boot should still continue (warning logged).

### R21-HW2 — `berry run` args + integer status (console scripts)

1. `berry run /flash/scripts/arg_test.be "argument 1" "asd asd asd argument 2"`
   - **Expect:** prints `1: argument 1` and `2: asd asd asd argument 2`; command returns success.
2. `berry run /flash/scripts/ping.be`
   - **Expect:** usage text; nonzero status (script-defined).
3. If network available: `berry run /flash/scripts/ping.be <host> [timeout_ms]`
   - **Expect:** existing ping contract preserved (reply/timeout/invalid args as before), status propagated.
4. `berry run /flash/scripts/hello.be`
   - **Expect:** script output, no crash.

### R21-HW3 — `berry exec` removed

1. `berry exec /flash/scripts/hello.be`
   - **Expect:** `berry: unknown operation 'exec'` and usage showing `berry run <path> [args...] ...` (no `exec`).

### R21-HW4 — UI app launch via `ui.launch` (TS-generated pattern preserved)

1. `berry run /flash/apps/tap_dash.be`
   - **Expect:** Tap Dash UI launches normally (same user experience as before R21).
2. Interact briefly (touch/input as available), then `berry close` or use app exit.
   - **Expect:** teardown/cleanup OK, console restored.
3. `berry run /flash/apps/breakout.be`
   - **Expect:** Breakout launches; `berry close` restores console.

### R21-HW5 — Default execution non-intrusive + explicit UI replacement

1. Launch a UI app: `berry run /flash/apps/tap_dash.be`.
2. From a context that can run a command without closing the app (if available in your setup) or validate via design/log inspection: running a non-UI `main(args)` script should not itself close/disturb the active UI app. **Practical check:** close app, run console scripts; confirm no UI lifecycle is triggered by console scripts.
3. Launch Breakout after closing Tap Dash (or let `ui.launch` replace as specified) — **Expect:** only the explicitly launched app is active; lifecycle is clean.

### R21-HW6 — R9 command-script resolution uses unified contract

1. Configure `path` as needed and place/resolve a `.be` command script with `main(args)`.
2. Invoke by bare name / resolved form used in R9.
3. **Expect:** args passed, integer status propagated, no UI disturbance. Native commands still take precedence.

### R21-HW7 — Error cases

1. `berry run /flash/does_not_exist.be` — **Expect:** file/read failure reported clearly, nonzero.
2. Script without `main(args)` (temporary test file): `def x() return 1 end` / `print(1)` then `berry run ...` — **Expect:** clear failure mentioning `main(args)` required / invalid arg; no crash.
3. `main(args)` returns non-integer (e.g. `"bad"`) — **Expect:** clear failure about integer shell status; no crash.
4. `main(args)` returns `nil` — **Expect:** treated as status `0`.

### R21-HW8 — Migrated hand-written test scripts

Run a representative sample to confirm `main(args)` migration:
- `berry run /flash/tests/r2/http_get.be` (or with args where applicable)
- `berry run /flash/tests/r2/repeated_fetch.be http://example.com/ 2 text` (if network available)
- `berry run /flash/tests/r5/memory_churn.be` then `berry profile`/`berry gc` as usual
- `berry run /flash/tests/r6/lazy_modules.be`
- `berry run /flash/tests/r7/include_body.be` (arg validation)
- `berry run /flash/tests/r8/http_validation.be` (if network/profile evidence desired)

**Expect:** scripts run (they no longer fail with “missing main”); behavior consistent with their purpose.

## Evidence / results

**Hardware validation (2026-11-09, user report):** all manual test steps passed.

| Case | Result | Notes |
|---|---|---|
| R21-HW1 Autoexec / console startup | ✅ OK | |
| R21-HW2 `berry run` args + integer status | ✅ OK | |
| R21-HW3 `berry exec` removed | ✅ OK | |
| R21-HW4 UI launch via `ui.launch` | ✅ OK | Both Breakout and Tap Dash launch; `berry close` OK |
| R21-HW5 Non-intrusive default + explicit UI replacement | ✅ OK | |
| R21-HW6 R9 command-script resolution (unified contract) | ✅ OK | |
| R21-HW7 Error cases | ✅ OK | |
| R21-HW8 Migrated hand-written test scripts | ✅ OK | |

Detailed serial logs/build identifier/board not attached in chat; pass recorded from user confirmation.

## Follow-ups if issues found

- Status propagation or `main` return-type edge cases: runtime `execute_main_entry`.
- UI lifecycle/console restore issues: `berry_runtime_ui_launch` / `close_active_app`.
- R9 resolver regressions: `shell_script_try_exec` / `shell_dispatch_expanded_line`.
- TS-generated UI/console scripts: `frontend/src/ide/berry/code-generator.ts` and regenerated `storage/flash_data/*.be`.
