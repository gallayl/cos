# I2C bus component review findings (2026-11-13)

**Review date:** 2026-11-13  
**Scope:** `components/i2c_bus/` (`include/i2c_bus.h`, `src/i2c_bus.c`, `src/i2c_bus_cmd.c`, `src/cyd_i2c_pins.h`, `CMakeLists.txt`)  
**Reviewer focus:** correctness, safety, scale, tests, speed, and lean review of the `i2c_bus` component as-is.  
**Load assumed:** Single device user. I2C operations are occasional: boot-time init from `main`, manual console commands (`i2c scan/read/write`), and possible infrequent API calls from other code. No concurrent callers assumed.

## Progress tracking

| ID | Severity | Finding | Status | Owner | Notes |
|---:|----------|---------|--------|-------|-------|
| B1 | Must fix | Scan/read/write use the bus before checking it was initialized | Fixed |  | 2026-11-13: `i2c_bus_scan/read/write` now return `ESP_ERR_INVALID_STATE` when `s_bus_handle` is NULL (scan sets `*found=0`). |
| B2 | Must fix | `i2c_bus_init` does not recover cleanly on repeat or partial failure | Open |  | Re-init can leak/replace handle; command registration aborts after bus is created |
| B3 | Should fix | Console command parsing turns bad addresses/bytes into device 0 or wrong values | Open |  | `strtol(..., NULL, 0)` / unchecked conversion; user typo targets wrong device |
| B4 | Nice to have | `i2c scan` can block for several seconds | Open |  | Probes 126 addresses sequentially with 50 ms timeout each |

**Status values:** Open → In progress → Fixed → Verified (add evidence/link/date in Notes).

## What this repo does

This repository is firmware for an ESP32 "CYD" board (ESP-IDF project with frontend/storage tooling around it). The audited `i2c_bus` component owns one I2C master bus on the CYD pins (SDA 27, SCL 22, 100 kHz), offers scan/read/write helpers, and registers an `i2c` console command for manual bus work. It is initialized once at startup from `main/main.c` and covered by host unit tests in `test/test_i2c_bus.c`.

## Findings

### Must fix

1. **Scan/read/write use the bus before checking it was initialized** (`src/i2c_bus.c:36-118`)
   - **What this is:** `i2c_bus_scan`, `i2c_bus_read`, and `i2c_bus_write` take the global `s_bus_handle` and pass it to ESP-IDF I2C calls (`i2c_master_probe`, `i2c_master_bus_add_device`).
   - **Problem:** The code never checks that `s_bus_handle` is valid. If a caller uses these APIs before `i2c_bus_init()`, or after init failed and the error was ignored, the handle is still `NULL`/invalid and is passed to driver calls.
   - **Fix:** Return a clear error (for example `ESP_ERR_INVALID_STATE`) when the bus handle is not ready. One guard in each public operation, or a small shared helper, is enough.
   - **If we skip it:** Early or failed-startup callers can make I2C operations fail in confusing ways or hit bad driver behavior on hardware instead of getting a normal error.

2. **`i2c_bus_init` does not recover cleanly on repeat or partial failure** (`src/i2c_bus.c:13-34`, `src/i2c_bus_cmd.c:105-114`)
   - **What this is:** `i2c_bus_init()` creates the master bus, saves it in `s_bus_handle`, registers the `i2c` console command, and logs success.
   - **Problem:** It does not check for an already-initialized bus, so a second call can replace `s_bus_handle` without deleting the old bus and can try to register the same console command again. Command registration uses `ESP_ERROR_CHECK(...)`, which aborts the program on failure; if that happens after `i2c_new_master_bus` succeeded, the bus is left created but init never reports success.
   - **Fix:** Make init safe to reason about: reject or no-op a repeat init, and roll back/delete the bus if later setup (like command registration) fails. Avoid aborting inside a library init path when an error can be returned to the caller.
   - **If we skip it:** Repeated init or a registration problem can leak the bus handle, crash on `ESP_ERROR_CHECK`, or leave the component half-ready.

### Should fix

3. **Console command parsing turns bad addresses/bytes into device 0 or wrong values** (`src/i2c_bus_cmd.c:42-99`)
   - **What this is:** The `i2c read` and `i2c write` subcommands parse the device address and byte values with `strtol(..., NULL, 0)` (and size with `atoi`) and cast results to `uint8_t`.
   - **Problem:** Invalid text is not detected: `strtol` returns `0` when it cannot parse, so a typo like `i2c read 0xZZ 4` or `i2c write nope 01` becomes address `0x00`. Values outside 0..255 are also truncated by the cast. The command then performs a real bus operation on the wrong target.
   - **Fix:** Parse with an end-pointer/range check (reject empty or trailing junk, require 7-bit address range as intended, require byte-sized data) and print usage on bad input instead of converting it to `0`.
   - **If we skip it:** A small typing mistake can read from or write to the wrong I2C device, which is hard to notice from the console.

### Nice to have

4. **`i2c scan` can block for several seconds** (`src/i2c_bus.c:36-56`)
   - **What this is:** `i2c_bus_scan()` probes addresses 1 through 126 one by one with a 50 ms timeout per probe.
   - **Problem:** When devices are missing or slow to answer, the scan can take up to about 6 seconds (`126 * 50 ms`) while the caller waits. In this project that means the console/user command blocks during the scan.
   - **Fix:** Keep the simple scan for now if acceptable, but consider a shorter probe timeout or a way to scan a narrower range when the expected devices are known.
   - **If we skip it:** Manual scans feel slow, but normal low-frequency console use still works.

## Verdict

Not healthy: fix B1 and B2 first so callers cannot use an unready bus and init cannot leak or abort after partial success.

## Lean

- `Lean: -0 lines, -0 dependencies possible.` No clear safe deletion or dependency removal found; read/write share small add/remove boilerplate, but extracting it is optional and not required for correctness.

## Validation / not checked

- 2026-11-13: Ran `make format-check` and `make test` after B1 fix — **pass** (host checks, including `test_i2c_bus`). Did not run ESP-IDF firmware build/lint or on-device I2C validation.
- Real ESP-IDF hardware behavior and timing (toolchain/device not exercised in this review).
- Thread/concurrency safety if scan/read/write/init are called from multiple tasks at once (review assumed single-user, occasional calls).
- Behavior of `i2c_master_probe` for bus errors vs "no device" on the target ESP-IDF version; scan currently treats any non-`ESP_OK` probe as "not found".
- Full console parsing/command behavior on device beyond static code reading; `i2c_bus_cmd.c` has no dedicated host tests in `test/test_i2c_bus.c`.

## Related code/tests

- Implementation: `components/i2c_bus/src/i2c_bus.c`, `components/i2c_bus/src/i2c_bus_cmd.c`, `components/i2c_bus/src/cyd_i2c_pins.h`, `components/i2c_bus/include/i2c_bus.h`
- Build: `components/i2c_bus/CMakeLists.txt`
- Existing host tests: `test/test_i2c_bus.c`
- Callers/context: `main/main.c` (`i2c_bus_init`)
