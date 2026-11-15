# System component review findings (2026-11-13)

**Review date:** 2026-11-13  
**Scope:** `components/system/` (`include/system.h`, `src/system.c`, `src/system_cmd.c`, `CMakeLists.txt`)  
**Reviewer focus:** correctness, safety, and maintainability of the `system` component as-is (senior-dev breakage review).  
**Load assumed:** One person using the serial console at a time; system/console commands run occasionally (not in a tight loop, not concurrently from many tasks).

## Progress tracking

| ID | Severity | Finding | Status | Owner | Notes |
|---:|----------|---------|--------|-------|-------|
| S1 | Must fix | Duplicate console command name `info` can abort boot | Fixed |  | 2026-11-13: Renamed system command to `sysinfo` in `components/system/src/system_cmd.c`; updated `test/test_system.c` to invoke `sysinfo`. Shell `info` (filesystem usage) remains unchanged. |
| S2 | Must fix | `esp_flash_get_size` failure ignored in `system_get_info` | Open |  | Decide error/unknown handling consistent with callers |
| S3 | Should fix | `system_format_uptime` rejects 3-byte buffer for `"0s"` | Fixed |  | 2026-11-13: `components/system/src/system.c` now accepts buffers >= 3 bytes; `snprintf` truncation check still rejects undersized buffers for actual output. |

**Status values:** Open → In progress → Fixed → Verified (add evidence/link/date in Notes).

## Findings

### S1 — Must fix: Duplicate console command name `info` crashes boot

- **Location:** `components/system/src/system_cmd.c:96`, `components/system/src/system_cmd.c:101`; collides with `components/shell/src/shell_info_cmds.c:53`, `components/shell/src/shell_info_cmds.c:58`; normal boot path `main/main.c:102-103`.
- **What this is:** `system_register_commands()` registers a console command named `"info"`. The shell registers a different command also named `"info"` (filesystem usage) in `shell_register_info_commands()` (called from `shell_init()`).
- **Problem:** ESP-IDF console registration fails for duplicate command names. Both registrations use `ESP_ERROR_CHECK`, which aborts the program on error. In the normal boot sequence `main.c` calls `system_init()` and then `shell_init()`, so the second `"info"` registration can abort startup.
- **Concrete case:** Boot with both components initialized → `system_init()` registers `"info"` → `shell_init()` attempts to register `"info"` again → `esp_console_cmd_register` fails → `ESP_ERROR_CHECK` aborts; device never reaches a usable shell.
- **Proposed fix:** Give this component's command a unique name (for example `sysinfo`) or remove the duplicate and keep one canonical implementation that exposes the needed fields. Update callers/tests that invoke `"info"` for this command. Prefer the smallest safe, compatible change.
- **If skipped:** Firmware can crash during initialization when both components are enabled.

### S2 — Must fix: Flash size read failure is hidden as a valid value

- **Location:** `components/system/src/system.c:33-35`.
- **What this is:** `system_get_info()` queries flash size via `esp_flash_get_size(NULL, &flash_size)` and stores it in `info->flash_size`.
- **Problem:** The `esp_err_t` return value is ignored. If the call fails, `flash_size` remains `0` but `system_get_info()` still returns `ESP_OK`, so `system_info_t` and the `info` command report `Flash size: 0 bytes` as if measured correctly.
- **Concrete case:** Flash driver/state issue causes `esp_flash_get_size` to fail → caller receives `ESP_OK` with `flash_size == 0` → diagnostics mislead debugging/support.
- **Proposed fix:** Check the result of `esp_flash_get_size`. On failure, avoid presenting `0` as a known-good size: propagate the error (and have callers report failure) or handle it in a way consistent with existing `system_get_info`/CLI expectations (e.g. log and surface "unknown"/error state). Choose behavior that matches current callers.
- **If skipped:** When flash size cannot be read, diagnostics show a wrong value.

### S3 — Should fix: `system_format_uptime` rejects a buffer that is sufficient for `"0s"`

- **Location:** `components/system/src/system.c:55-58`, contract noted in `components/system/include/system.h:46-51`.
- **What this is:** `system_format_uptime()` validates buffer capacity before formatting uptime into `buf`.
- **Problem:** The function rejects `len < 4`. At uptime 0 the formatted output is `"0s"` (2 chars + NUL = 3 bytes), so a 3-byte buffer is sufficient but is rejected with `ESP_ERR_INVALID_SIZE`. The header recommends `>= 24` but the implementation enforces a stricter minimum than necessary.
- **Concrete case:** `mock_system_set_uptime_us(0); char buf[3]; system_format_uptime(buf, sizeof(buf));` returns `ESP_ERR_INVALID_SIZE` even though `"0s"` would fit.
- **Proposed fix:** Allow the smallest valid output size (3 bytes) and rely on the existing `snprintf` result/truncation check (`written < 0 || (size_t)written >= len`) to reject buffers too small for the actual formatted string. Consider adding a unit test for the 3-byte `"0s"` case.
- **If skipped:** Callers with a tight but sufficient buffer for very small uptimes get a spurious failure.

## Validation / not checked

- 2026-11-13: Ran `make format-check` and `make test` after S1/S3 fixes — **pass** (host checks, including `test_system`). Did not run ESP-IDF firmware build/lint, QEMU, or on-device boot validation.
- Full ESP-IDF device build and boot run of the combined firmware after resolving S1 (ESP-IDF/toolchain not exercised in this review).
- Behavior of `esp_flash_get_size` failure paths on real hardware (S2).

## Related code/tests

- Implementation: `components/system/src/system.c`, `components/system/src/system_cmd.c`, `components/system/include/system.h`
- Existing host tests: `test/test_system.c`
- Collision context: `components/shell/src/shell_info_cmds.c`, `components/shell/src/shell.c`, `main/main.c`
