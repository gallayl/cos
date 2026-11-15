# R20 Stderr/FD Redirection — Parse + Best-Effort Runtime Design

**Date:** 2026-11-10 (updated)  
**Status:** Parse/diagnostic foundation + best-effort runtime implemented (stdio fd/`FILE*` redirection; ESP-IDF log-capture semantics deferred)  
**Scope:** Recognize and execute common stderr/FD redirection operators (`2>`, `2>>`, `&>`, `&>>`) by redirecting `stdout`/`stderr` streams/fds to VFS targets. This captures output written via redirected stdio streams/fds; `ESP_LOGx` routing may differ by console/log configuration and is out of scope.

## Goals

- Detect FD-specific/combined redirection syntax safely (quote-aware) so it is not mis-parsed as plain stdout redirection.
- Execute supported `2>`, `2>>`, `&>`, `&>>` forms as **best-effort stdio fd/`FILE*` redirection** (same VFS target resolution/path rules as `>`/`>>`).
- Preserve existing `>` / `>>` stdout redirection behavior for plain stdout forms.
- Cover parser/dispatch/runtime-path cases with host unit tests; keep manual hardware validation minimal.

## In scope (this slice)

- Quote-aware detection of unquoted operators:
  - `2>`  — stderr truncate/redirect (recognized, unsupported runtime)
  - `2>>` — stderr append (recognized, unsupported runtime)
  - `&>`  — stdout+stderr truncate/redirect (recognized, unsupported runtime)
  - `&>>` — stdout+stderr append (recognized, unsupported runtime)
- Attached and spaced forms (e.g. `cmd 2>err.log`, `cmd 2> err.log`).
- Diagnostics:
  - recognized stderr/FD/combined redirection syntax → `stderr/fd redirection: not supported yet` (handled, non-zero)
- Plain stdout `>` / `>>` behavior unchanged when not part of a recognized FD/combined operator.

## Out of scope (deferred)

- Runtime stderr/fd duplication/redirection implementation.
- Semantics of ESP-IDF/console logging vs process stderr (many logs may be emitted via stdout/UART/log APIs).
- Explicit `1>` / `1>>` normalization, arbitrary fd numbers (`3>`, etc.), read redirection (`<`, `2<`), here-docs, `>&`/`<&` duplication operators, and multiple simultaneous redirections.
- Combined pipeline + stderr runtime semantics.

## Parsing/dispatch behavior

- `shell_parse_pipeline()` still runs first; pipe syntax remains diagnosed as before.
- A new parse/scan detects unquoted stderr/FD/combined redirection operators (`2>`, `2>>`, `&>`, `&>>`) before stdout redirection parsing/execution.
  - If found: dispatch reports `stderr/fd redirection: not supported yet` and returns handled/non-zero.
- `shell_parse_redirection()` (stdout `>`/`>>`) ignores `>` characters that are part of these recognized FD/combined operators so `cmd 2> file` is not misinterpreted as stdout redirection.

## Examples

| Input | Result |
|---|---|
| `cmd > out.txt` | existing stdout redirection behavior |
| `cmd >> out.txt` | existing stdout redirection behavior |
| `cmd 2> err.txt` | `stderr/fd redirection: not supported yet` |
| `cmd 2>>err.txt` | `stderr/fd redirection: not supported yet` |
| `cmd &> all.txt` | `stderr/fd redirection: not supported yet` |
| `cmd &>> all.txt` | `stderr/fd redirection: not supported yet` |
| `echo "2> x"` | no stderr/fd redirection operator detected (quoted) |
| `cmd 2> err.txt > out.txt` | diagnosed as unsupported stderr/fd redirection (no partial execution) |

## Implementation notes (2026-11-10, best-effort runtime)

- Unified parsing in `shell_parse_redirection()` recognizes `2>`, `2>>`, `&>`, `&>>` and produces `SHELL_REDIRECT_STDERR_*` / `SHELL_REDIRECT_BOTH_*` types (target/command extraction and "single trailing redirection" rules same as stdout forms).
- `shell_redirect_begin/end` save/redirect/restore `stdout` and/or `stderr`:
  - fd path: `dup` saved fd + `dup2` target fd when `fileno(stream)`/`fileno(target)` are usable.
  - fallback (non-Windows/ESP-IDF): swap `stdout`/`stderr` `FILE*` pointers when fd redirection is unavailable.
  - `&>`/`&>>` open one target file and share it for both streams.
- Best-effort scope: captures writes to redirected stdio streams/fds. `ESP_LOGx` may be emitted via console/UART/vprintf routing not equivalent to fd 2; **log-capture aware redirection is deferred**.
- `shell_parse_fd_redirection()` remains for compatibility as a syntax detector; dispatch uses unified parse/runtime instead of rejecting recognized fd forms.

## Validation

- Host unit tests (`test/test_shell_script.c`, 33/33): fd/stderr/combined parsing (`2>`, `2>>`, `&>`, `&>>`, attached/quoted), detector compatibility, dispatch runtime-path handling (no "unsupported fd redirection" diagnostic), regressions for pipes and stdout redirection. Related suites: `test_shell` 63/63, `test_shell_berry` 28/28.
- Firmware build: `ninja -C build` (ESP-IDF 6.1) succeeded, `cos.bin` `0x1d9bb0`.
- **Manual hardware smoke: pending/minimal (optional).** Suggested on `/sdcard` or `/flash` with a command that emits to stdio stdout/stderr as applicable:
  - `cmd 2> ./err.log` / `cmd 2>> ./err.log` (verify file content and console restored)
  - `cmd &> ./all.log` (verify combined capture where streams hit fd/stdio)
  - confirm normal `cmd > ./out.log` still works
  Note: ESP-IDF logs may not be captured by fd/stdio redirection depending on log sink/config. Record evidence in plan/report if run.

## Future

- ESP-IDF log-capture aware redirection (`esp_log_set_vprint`/routing) with clear concurrency/restore guarantees (separate milestone).
- Multiple/combined target redirections (e.g. separate stdout+stderr targets) and integration with future pipe runtime semantics.