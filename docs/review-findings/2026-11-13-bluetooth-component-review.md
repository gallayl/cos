# Bluetooth component review findings (2026-11-13)

**Review date:** 2026-11-13  
**Scope:** `components/bluetooth/` (`include/bluetooth.h`, `src/bluetooth.c`, `src/bluetooth_cmd.c`, `src/bluetooth_hid.c`, `src/bluetooth_hid.h`, `CMakeLists.txt`)  
**Reviewer focus:** correctness, safety, concurrency/resource handling, and maintainability of the Bluetooth (BLE HID host) module as-is.  
**Load assumed:** Single user on the serial console with occasional `bt` commands, one BLE HID keyboard at a time, and BLE stack callbacks/events running on ESP-IDF Bluetooth/HID tasks.

## Progress tracking

| ID | Severity | Finding | Status | Owner | Notes |
|---:|----------|---------|--------|-------|-------|
| B1 | Must fix | Shared Bluetooth state and scan results are accessed from multiple tasks without synchronization | Open |  | Console, `bt_enable`, GAP, and HID callbacks can race on flags, scan cache, and device handle |
| B2 | Should fix | Unknown BLE address type can break manual connect and saved auto-reconnect | Open |  | `lookup_addr_type` fallback to public + saving type 0 can fail for random/static-random keyboards |
| B3 | Should fix | `bt forget` does not fully remove pairing when Bluetooth is off and can race a slow disconnect | Open |  | Bonds in controller/NVS are only removed while enabled; timeout path still erases auto device |
| B4 | Nice to have | `bluetooth_init` leaks semaphores on partial allocation failure and HID callback reads are not snapshotted | Open |  | Small init/error-path and callback-robustness cleanup |

**Status values:** Open → In progress → Fixed → Verified (add evidence/link/date in Notes).

## Findings

### B1 — Must fix: Shared Bluetooth state and scan results are accessed from multiple tasks without synchronization

- **Location:** `components/bluetooth/src/bluetooth.c:49-60`, `131-137`, `140-275`, `277-337`, `340-473`, `514-757`; `components/bluetooth/src/bluetooth_hid.c:27-42`, `203-314`
- **What this is:** The module keeps global state for scan results (`s_scan_results`, `s_scan_count`), enable/connect flags (`s_enabled`, `s_enable_in_progress`, `s_hidh_inited`), and the active HID device (`s_hid_dev`). Console commands run on the caller task, enable runs in `bt_enable` task, and BLE GAP/HID events run in ESP-IDF callback/event contexts.
- **Problem:** These fields are read and written without a mutex or other synchronization (most are plain globals; some are only `volatile`). `bluetooth_scan()` clears and later prints scan results while GAP callbacks can update them; `lookup_addr_type()` and auto-connect injection read/write the same array; `s_hid_dev` is checked/set/cleared across console APIs and HID open/close callbacks.
- **Concrete case:** User runs `bt scan` and a GAP scan-result/completion callback updates `s_scan_results`/`s_scan_count` at the same time the console path resets or prints them → stale, duplicated, or out-of-bounds-looking output/state. Or `bt connect`/status runs while a HID close/open callback changes `s_hid_dev` → connect/disconnect decisions and reported device name/BDA can observe a half-updated or freed/closing device handle.
- **Fix:** Define one lock-protected state model (or confine all BT state mutations to one task/queue). Protect scan cache, flags, and the current device pointer together; copy out data needed for printing/status; and make close/connect transitions explicit so callbacks and console commands cannot interleave unsafely.
- **If we skip it:** Occasional hard-to-reproduce crashes, wrong device status, bad reconnect target, or use-after-close behavior under normal console + callback timing.

### B2 — Should fix: Unknown BLE address type can break manual connect and saved auto-reconnect

- **Location:** `components/bluetooth/src/bluetooth.c:125-137`, `291-301`, `447-469`, `631-665`
- **What this is:** `bluetooth_connect()` looks up the peer address type from the last scan results and falls back to `BLE_ADDR_TYPE_PUBLIC`. On successful HID open it saves the auto-connect device/BDA plus the looked-up address type to NVS. On enable, it restores the saved BDA/type and calls `bluetooth_connect()`.
- **Problem:** If the address type is not known (device not present in current scan cache, manual connect without a matching scan entry, or stale NVS/type), the code uses/saves public/type `0`. Many BLE keyboards use random or resolvable-private/static-random address types, so the wrong type can prevent `esp_hidh_dev_open` from connecting.
- **Concrete case:** User pairs/connects a keyboard that advertises with a random address type but the scan cache does not contain its entry when connect/open saves auto info (or user runs `bt connect <addr>` directly) → NVS stores type `0`, auto-reconnect after reboot calls open with public type and fails; manual reconnect may also fail until a scan rediscovers the correct type.
- **Fix:** Treat unknown address type as "not known" instead of forcing public: require a recent scan result, or try plausible BLE address types / preserve a valid stored type. Do not overwrite a previously known-good stored address type with `0` when lookup fails.
- **If we skip it:** Some keyboards will connect once but fail to reconnect automatically (or manually without rescanning), making pairing unreliable.

### B3 — Should fix: `bt forget` does not fully remove pairing when Bluetooth is off and can race a slow disconnect

- **Location:** `components/bluetooth/src/bluetooth.c:677-719`, `549-560`
- **What this is:** `bluetooth_forget()` optionally disconnects the active HID device, removes saved auto-connect keys from NVS, and (only when `s_enabled`) removes bonded devices from the BLE stack.
- **Problem:** When Bluetooth is disabled, bonded keys stored by the controller/stack are not removed, but the saved auto-connect record is erased and the command reports success. When a disconnect is in progress, the function waits up to `BT_CLOSE_TIMEOUT_MS` and then continues even if the close semaphore is not received.
- **Concrete case:** User runs `bt off` (or device is currently off) then `bt forget` to fully unpair → NVS auto device is cleared but BLE bond keys remain, so the stack can retain pairing state unexpectedly. Or disconnect close event is delayed past 3s → `bluetooth_forget()` removes bonds/records while the device handle/callbacks may still be finishing close.
- **Fix:** Make forget semantics explicit and complete: if not enabled, either refuse with a clear message or temporarily enable/access the bond store as required to remove keys; on disconnect timeout, return/report failure instead of mutating bond/NVS state as if cleanup completed.
- **If we skip it:** Users can think a keyboard is fully forgotten when bonds remain, or cleanup can interleave with an in-flight disconnect.

### B4 — Nice to have: `bluetooth_init` leaks semaphores on partial allocation failure and HID callback reads are not snapshotted

- **Location:** `components/bluetooth/src/bluetooth.c:483-516`; `components/bluetooth/src/bluetooth_hid.c:30-42`, `203-314`
- **What this is:** `bluetooth_init()` allocates three binary semaphores and then registers commands / may auto-start. HID input processing reads global callback pointers and invokes them.
- **Problem:** If `xSemaphoreCreateBinary()` fails for the second or third semaphore, `bluetooth_init()` returns `ESP_ERR_NO_MEM` without deleting semaphores already created. In HID input, callbacks are checked and then called from the global volatile pointers (and in some branches read again), so a concurrent update/clear could race the check/call.
- **Concrete case:** Low-memory init path fails creating `s_scan_params_set` or `s_close_done` → `s_scan_done` (and possibly `s_scan_params_set`) is leaked. Or one task clears a keyboard callback while the HID callback task is dispatching events → dispatch can observe a cleared pointer between check and call.
- **Fix:** Free any semaphores created before returning init failure. In HID dispatch, copy callback function pointers to locals once and invoke the locals.
- **If we skip it:** Small one-time memory leak on a failed init path and a narrow race during callback registration/teardown.

## Not checked

- On-device behavior with ESP-IDF v6.1 BLE/HID stack (pairing, reconnect timing, scan results, and controller bond storage semantics were reasoned from code).
- Full interaction with `input`/Berry `ui.key_down`, `main` keyboard routing, and concurrent shell/Berry usage beyond static call-site review.
- Host unit tests for this module were not run in this audit pass; existing coverage is `test/test_bluetooth_cmd.c`, `test/test_bluetooth_hid.c`, and `test/mocks/mock_bluetooth.*` (core `src/bluetooth.c` is not covered by those host tests).
- Memory/stack sizing of BLE tasks and `bt_enable` task under real load.

## Lean

- `Lean: -0 lines, -0 dependencies possible.` (no safe deletions or dependency removals identified without changing behavior/contracts; main size/concurrency issues need refactoring, not removal.)
