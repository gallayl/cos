# R20 Redirection Runtime Wiring — Hardware Validation Guide (`>` / `>>`)

**Date:** 2026-11-10  
**Scope:** Runtime stdout redirection for `>` (truncate/create) and `>>` (append/create), wired through `shell_dispatch_expanded_line()` using `shell_parse_redirection()` / `shell_redirect_begin|end`.  
**Prerequisite:** Firmware built from current workspace and flashed to device (ESP-IDF build not performed in this agent environment).

## What should work

- Native commands and script commands dispatched through the shell can redirect stdout to a VFS file.
- `>` overwrites/creates file; `>>` appends/creates file.
- Target paths may be relative to current shell CWD (e.g. `./info.log`) or absolute virtual paths (e.g. `/sdcard/info.log`, `/flash/info.log`).
- Quoted targets supported by parser (e.g. `cmd > "my file.txt"`).

## Limitations / expectations

- Stdout only (no stderr/FD-specific redirection, no pipes).
- Single trailing redirection only; malformed/unsupported syntax reports `redirection: invalid or unsupported syntax`.
- Redirect applies to command execution output; shell prompt/echo remain on console.
- Runtime failures (cannot resolve/open target, invalid path, target is directory, etc.) should print a console error like `redirection: cannot open '<target>': ...` and return non-zero.

## Test procedure (on device)

From a writable directory (e.g. `/sdcard`):

1. **Baseline**
   - `cd /sdcard`
   - `rm info.log` if present (or choose a new filename)
   - `ls` and confirm `info.log` absent.

2. **Truncate/create (`>`)**
   - `info > ./info.log`
   - Expected: no `info` output on console (or minimal shell feedback only), command succeeds.
   - `ls` shows `info.log` with non-zero size.
   - `cat ./info.log` shows expected `info` output.

3. **Append (`>>`)**
   - `info >> ./info.log`
   - `cat ./info.log` shows previous content followed by appended `info` output (file larger than after step 2).

4. **Absolute path / different filesystem (if available)**
   - `info > /flash/r20_info.log` (or `/sdcard/...` absolute)
   - `cat /flash/r20_info.log` shows content.

5. **Quoted/attached forms (as applicable)**
   - `info>./info2.log` and/or `info >> ./info3.log`
   - `echo "a > b"` should print normally (quoted operator not treated as redirection) — optional sanity check.

6. **Error cases**
   - `info >` -> syntax error reported on console, no crash.
   - `info > ./no_such_dir/out.log` (if parent missing) -> open/path error reported on console.
   - `info > /sdcard` (directory target) -> rejected/error reported.
   - After any error, run a normal command (e.g. `info`) to confirm console stdout is restored.

7. **Script command (optional, R9 context)**
   - Run a known script command with redirection, e.g. `my_script args > ./script.log` if available, and verify output captured/restored.

## Record results

Capture for each case: command, console result, file created/size/content (`cat`), and any errors. Note device, storage used (`/flash` vs `/sdcard`), and firmware build/flash details if available. Update `docs/plan/consolidated-improvement-plan.md` R20 progress/log with evidence (pass/fail, issues). If failures occur, include exact console output and whether stdout was left redirected.

## Hardware validation results (2026-11-10, user)

- **Status:** ✅ **Passed (basic `>` / `>>` runtime + hardening follow-up checks)**
- **Storage:** `/sdcard` primary, `/flash` target checked

### Initial basic pass
- `info > ./info.log` produced no console body output, created `info.log` (122 bytes), and `cat ./info.log` contained expected `info` output.
- `info >> ./info.log` appended output; `cat ./info.log` showed previous content followed by appended `info` output.
- Console/stdout restored correctly for subsequent commands.

### Follow-up hardening pass (transcript provided)
- **Setup note:** `rm ... 2>/dev/null` correctly failed because `/dev/null` is not a COS virtual path (`redirection: '/dev/null' is not a virtual path` / `ESP_ERR_INVALID_ARG`); not a redirection defect.
- **Syntax/parse errors:** `info >` and `echo hi > a > b` reported `redirection: invalid or unsupported syntax`, no crash; subsequent `info` printed normally (stdout restored). ✅
- **Missing parent dir:** `info > ./r20_hw_nodir/out.log` reported `fopen('/sdcard/r20_hw_nodir/out.log', "w") failed` and `cannot redirect stdout...: ESP_FAIL`, no crash; subsequent `info` printed normally. ✅
- **Directory targets:** `info > ./r20_hw_dir` and `info > /sdcard` reported directory-target rejection and `cannot redirect stdout...: ESP_ERR_INVALID_ARG`, no crash; subsequent `info` printed normally. ✅
- **Attached/quoted forms:** `info>./r20_hw_1.log` created/captured output, `info >> ./r20_hw_1.log` appended correctly, `info > "./r20_hw_2.log"` captured correctly, `cat` showed expected content. ✅
- **Quoted operator sanity:** `echo "a > b"` produced `Unknown command: echo "a > b"` (no `echo` command in this environment) and was **not** treated as redirection, as intended. ✅
- **`/flash` target:** `info > /flash/r20_hw_flash.log` succeeded; `ls /flash` showed file; `cat /flash/r20_hw_flash.log` showed expected content. ✅
- **Script command redirection:** `berry run /flash/scripts/hello.be > ./r20_hw_script.log` captured `berry: execution failed (ESP_ERR_NOT_FOUND)` into `r20_hw_script.log` (44 bytes) and restored console for subsequent commands. Redirection/capture behaved correctly; script-path availability/execution result is separate from redirection. ✅ (capture/restore), script execution itself not verified as successful in this run.
- **Additional evidence (user, R9-resolved script with args):** from `/sdcard/scripts`, `arg_test 123 123 > arg.txt` created `arg.txt` containing Berry runtime INFO logs and script stdout (`I (...) berry: Loaded Berry file...`, `[R5] ...`, `1: 123`, `2: 123`); console restored and `cat arg.txt` printed captured content. This confirms redirection works for script-resolved commands with arguments and captures stdio output emitted during execution (including Berry logs routed to stdout in this configuration). ✅
- **Final sanity:** final `info` and `ls` behaved normally; redirected files present with expected sizes/content.

**Conclusion:** R20 basic `>`/`>>` redirection is functionally validated for common use, append, quoted/attached forms, cross-filesystem targets (`/sdcard`, `/flash`), error handling, and stdout restoration. Remaining broader scope (pipes, stderr/FD-specific/multiple redirections, advanced shell semantics) still deferred.