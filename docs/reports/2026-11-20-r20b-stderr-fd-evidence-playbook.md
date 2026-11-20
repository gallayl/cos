# R20b Stderr/FD Redirection — Evidence, HW Smoke Matrix, and Logging Capture Playbook

**Date:** 2026-11-20  
**Scope:** Best-effort stderr/FD stdio redirection (`2>`, `2>>`, `&>`, `&>>`) — canonical hardware smoke matrix, one end-to-end shell session playbook, and documentation of captured vs not-captured output.  
**Related:** [`2026-11-10-r20-stderr-redirection-parsing-design.md`](./2026-11-10-r20-stderr-redirection-parsing-design.md), [`2026-11-10-r20-runtime-pipes-stderr-design-spike.md`](./2026-11-10-r20-runtime-pipes-stderr-design-spike.md), [`2026-11-10-r20-redirection-runtime-hw-guide.md`](./2026-11-10-r20-redirection-runtime-hw-guide.md), [`../plan/ready/r20-stderr-fd-evidence-logging-playbook.md`](../plan/ready/r20-stderr-fd-evidence-logging-playbook.md)

## Summary / contract (stdio-fd best-effort)

- **Supported operators:** `2>` (stderr, truncate/create), `2>>` (stderr, append/create), `&>` (stdout+stderr, truncate/create, shared target), `&>>` (stdout+stderr, append/create, shared target). Spaced and attached forms are supported (e.g. `cmd 2>err.log`, `cmd &>> all.log`). Targets follow the same VFS path resolution/rules as `>`/`>>` (relative to shell CWD or absolute virtual paths such as `/flash/...`, `/sdcard/...`).
- **Captured:** output written to redirected **stdio streams/file descriptors** (`stdout`/fd 1 and/or `stderr`/fd 2 as applicable) via the shell's best-effort redirection mechanism (`dup`/`dup2` when `fileno` usable, with `FILE*` swap fallback on non-Windows/ESP-IDF paths as implemented). For `&>`/`&>>`, stdout and stderr share the **same target file**.
- **Not guaranteed / caveat:** **`ESP_LOG*` output is not guaranteed to be captured.** ESP-IDF logs may be emitted via console/UART/vprintf routing that is not equivalent to redirected stdio fd 2/1 depending on configuration/build; log-capture-aware redirection is out of scope (deferred).
- **Limits (unchanged in spirit):** single trailing redirection only; multiple redirections are rejected as invalid/unsupported syntax (no partial execution). Path/target resolution and open errors are reported on console with non-zero status and streams restored (e.g. invalid/non-virtual path, missing parent directory/open failure, directory target). Quote-aware parsing ignores operators inside quotes. Pipes + redirection combinations remain unsupported per current pipe/redirection handling.
- **Flush/ordering:** best-effort. Streams are flushed/restored around command execution as implemented; interleaving/ordering between stdout and stderr producers is not strictly guaranteed beyond stdio/runtime behavior.

## Canonical HW smoke matrix (CYD, required for evidence)

**Status:** **Not executed (hardware unavailable).** Blockers documented per `AGENTS.md` / validation policy: no CYD device/serial console access available in this environment; SD card presence/mount state unknown/unavailable for `/sdcard` validation. Firmware build/flash and on-device interaction cannot be performed here. A human/maintainer must run the matrix on a flashed CYD with console access and record results.

### Environment prerequisites
- Flashed firmware including current shell redirection runtime; console access (UART/serial monitor).
- Writable `/flash` available. `/sdcard` inserted/mounted and writable for SD-card cases (or document if unavailable).
- Choose a clean working directory per filesystem (e.g. `/flash` and `/sdcard`) and remove/avoid name collisions with prior test files.

### Matrix

| ID | Filesystem | Case | Commands to run (copy-paste, adapt path prefix) | Expected file checks | Observations to record |
|---|---|---|---|---|---|
| M1 | `/flash` | stdout truncate (`>`) baseline | `cd /flash`<br>`info > r20b_flash_out.log`<br>`ls r20b_flash_out.log`<br>`cat r20b_flash_out.log` | File created, non-zero when `info` emits stdout; `cat` shows expected `info` output (no console body output captured except prompt/diagnostics as applicable). | Command result/status, file size/content, console restored. |
| M2 | `/flash` | stdout append (`>>`) sanity | `info >> r20b_flash_out.log`<br>`cat r20b_flash_out.log` | File larger than M1; contains previous content followed by appended output. | Append vs truncate behavior, console restored. |
| M3 | `/flash` | stderr truncate (`2>`) | `info 2> r20b_flash_err.log`<br>`ls r20b_flash_err.log`<br>`cat r20b_flash_err.log` | File created. Captures only stdio-stderr output from command (may be empty if command writes no stderr fd output). Stdout behavior follows normal console unless separately redirected. | File content (empty/non-empty) and which stream appeared on console vs file. |
| M4 | `/flash` | stderr append (`2>>`) | `info 2>> r20b_flash_err.log`<br>`cat r20b_flash_err.log` | Appends stdio-stderr output to existing file (or creates). Append/truncate sanity consistent with `>>`/`2>`. | Append behavior, console restored. |
| M5 | `/flash` | combined truncate (`&>`, shared target) | `info &> r20b_flash_all.log`<br>`ls r20b_flash_all.log`<br>`cat r20b_flash_all.log` | Single shared target file contains stdio stdout and/or stderr produced by command (best-effort, fd/stdio only). | Shared-file behavior, content/interleaving notes, console restored. |
| M6 | `/flash` | combined append (`&>>`, shared target) | `info &>> r20b_flash_all.log`<br>`cat r20b_flash_all.log` | Appends combined stdio streams to same shared file. | Append behavior, console restored. |
| M7 | `/flash` | `ESP_LOG*` observation | Run a command/case that produces visible `ESP_LOG*` on console (if available in build/config), redirected with `2>`, `&>`, or `>` as appropriate; compare console vs file. | **Expectation:** `ESP_LOG*` may appear on console/UART but **not** in redirected file depending on log sink/routing. | Record whether any `ESP_LOG*` output appeared in redirected files (expected: often no; not guaranteed). |
| M8 | `/sdcard` | Repeat M1–M7 on `/sdcard` | `cd /sdcard` and repeat cases with `r20b_sd_*` filenames (or equivalent). | Same expectations on writable/mounted SD card. | SD mount/availability, any path/storage-specific differences, and M7 `ESP_LOG*` observation. |

**Also record (as observed):** invalid/malformed forms rejected without crash (e.g. missing target/command, multiple redirections), target-is-directory/missing-parent/open errors reported with non-zero status, and console stdout/stderr restored after each case/error. These are covered by prior `>`/`>>` HW evidence and host tests for parse/dispatch; recheck opportunistically during smoke.

## One end-to-end shell session example (copy-paste playbook)

**Execution status:** **Not executed on hardware** (blockers above). Syntax/structure verified against in-repo redirection behavior (`components/shell/src/shell_redirection.{h,c}`, `shell_script.c`) and existing `>`/`>>` hardware evidence in [`2026-11-10-r20-redirection-runtime-hw-guide.md`](./2026-11-10-r20-redirection-runtime-hw-guide.md); `2>`, `2>>`, `&>`, `&>>` parse/dispatch/runtime paths are covered by host unit tests (`test/test_shell_script.c`, 33/33 at implementation time) but on-device stdio/`ESP_LOG*` routing must be confirmed by a human.

Run on a writable location (`/flash` or `/sdcard`). Replace `<fs>` with `/flash` or `/sdcard` as needed.

```text
# 1) Prepare working directory (writable filesystem)
cd <fs>
pwd
ls

# 2) Stdout redirection: truncate/create, verify, then append
info > r20b_demo_out.log
ls r20b_demo_out.log
cat r20b_demo_out.log
info >> r20b_demo_out.log
cat r20b_demo_out.log
# Expected: first cat shows info output captured from stdio stdout; second shows prior + appended output (append/truncate sanity).

# 3) Stderr redirection: truncate/create then append (stdio fd 2 only)
info 2> r20b_demo_err.log
ls r20b_demo_err.log
cat r20b_demo_err.log
info 2>> r20b_demo_err.log
cat r20b_demo_err.log
# Expected: captures only stdio-stderr output produced by the command (may be empty if none). Console stdout remains visible unless redirected. ESP_LOG* is not guaranteed to appear here.

# 4) Combined stdout+stderr to one shared target: truncate/create then append
info &> r20b_demo_all.log
ls r20b_demo_all.log
cat r20b_demo_all.log
info &>> r20b_demo_all.log
cat r20b_demo_all.log
# Expected: both stdio stdout and stdio stderr (fd 1/2) go to the SAME file (best-effort). Append form appends to that shared file.

# 5) Post-check: console restored
info
# Expected: normal console output; redirection did not persist.
```

**Notes for the human runner**
- `info` is a representative command with stdout output; substitute another command as appropriate to exercise stdio-stderr producers when available. If a redirected file is empty, that indicates no output was written to the redirected stdio stream/fd by that command under current routing.
- **`ESP_LOG*` caveat:** logs may be routed via UART/console/vprintf and bypass redirected stdio fds; do not assume `2>`/`&>` captures `ESP_LOG*`. Record actual observations (M7).
- **`&>`/`&>>` callouts:** shared target file for both streams; stdio/fd-only scope; `ESP_LOG*` may bypass/not guaranteed; existing limits apply (single trailing redirection; multiple redirections rejected; path/dir/open errors handled as before in spirit); flush/ordering best-effort.
- Repeat the session on both `/flash` and `/sdcard` (when mounted/writable) to satisfy the canonical matrix.

## Explicit callouts (`&>` / `&>>`)

- **(a) Shared target file:** `&>` and `&>>` open one target and redirect both `stdout` and `stderr` to that same file (truncate/create or append/create respectively).
- **(b) Scope and `ESP_LOG*`:** redirection applies to **stdio/fd producers** (`stdout`/`stderr` streams, fd 1/2). **`ESP_LOG*` may bypass or be routed differently** (console/UART/vprintf/log sink) and **is not guaranteed** to appear in redirected files.
- **(c) Existing limits:** multiple redirections in one command are rejected (invalid/unsupported syntax, no partial execution). Path resolution, non-virtual/invalid targets, missing parent/open failures, and directory targets behave consistently with current redirection/path handling (reported errors, non-zero status, streams restored).
- **(d) Flush/ordering expectations:** best-effort; output ordering/interleaving depends on when producers write/flush to stdio streams and runtime behavior, and is not strictly guaranteed.

## Related documentation (existing)

- Runtime/parse design and limits: [`2026-11-10-r20-stderr-redirection-parsing-design.md`](./2026-11-10-r20-stderr-redirection-parsing-design.md)
- Feasibility/runtime spike (pipes/stderr): [`2026-11-10-r20-runtime-pipes-stderr-design-spike.md`](./2026-11-10-r20-runtime-pipes-stderr-design-spike.md)
- `>`/`>>` hardware validation evidence (baseline behavior): [`2026-11-10-r20-redirection-runtime-hw-guide.md`](./2026-11-10-r20-redirection-runtime-hw-guide.md)

## Blockers / required human actions

- **Blocking (hardware/manual):** execute canonical HW smoke matrix on CYD for both `/flash` and `/sdcard` and record results (commands, produced files/content, `ESP_LOG*` observations, any deviations). Requires flashed device, serial/console access, and mounted/writable SD card for SD cases.
- **Remediation:** build/flash firmware per `README.md`/`AGENTS.md` (ESP-IDF v6.1 environment), connect to device console, run the matrix/playbook above, and append evidence (device/build/storage details, transcripts, file contents/sizes, pass/fail) to this report or a linked follow-up report. If SD is unavailable, document that explicitly and complete `/flash` evidence.
