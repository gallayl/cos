# WiFi component review findings (2026-11-13)

**Review date:** 2026-11-13  
**Scope:** `components/wifi/` (`include/wifi.h`, `src/wifi.c`, `src/wifi_cmd.c`, `src/wifi_default_config.h`, `CMakeLists.txt`)  
**Reviewer focus:** correctness, safety, resource handling, and maintainability of the WiFi component as-is.  
**Load assumed:** Single user at a time on the serial console; WiFi commands and connect/scan actions run occasionally (not from many tasks at once).

## Progress tracking

| ID | Severity | Finding | Status | Owner | Notes |
|---:|----------|---------|--------|-------|-------|
| W1 | Must fix | `wifi_shutdown`/`wifi_init` do not fully release or restore ESP-IDF WiFi resources | Open |  | Reinit leaks handles on real hardware; contract says init is allowed after shutdown |
| W2 | Must fix | `wifi_init` leaves the component stuck after a mid-init failure | Open |  | `s_event_group` stays set but WiFi is only partly started |
| W3 | Should fix | `wifi scan` can fail in AP-only startup mode | Open |  | Scan requires STA in common ESP-IDF setups; AP-only is the default when no saved credentials |

**Status values:** Open → In progress → Fixed → Verified (add evidence/link/date in Notes).

## Findings

### W1 — Must fix: `wifi_shutdown`/`wifi_init` do not fully release or restore ESP-IDF WiFi resources

- **Location:** `components/wifi/src/wifi.c:57-123`, `components/wifi/src/wifi.c:125-140`
- **What this is:** `wifi_init()` creates the event group, initializes netif/event loop, creates default STA/AP netifs, initializes `esp_wifi`, and registers WiFi/IP event handlers. `wifi_shutdown()` disconnects, stops WiFi, and clears local pointers/state.
- **Problem:** `wifi_shutdown()` does not delete `s_event_group`, does not unregister the event handler instances, and does not deinitialize or destroy the WiFi/netif objects it created. The handler instance IDs are stored only in local variables in `wifi_init()` (`wifi_inst`, `ip_inst`) so they cannot be unregistered later. `wifi_init()` always creates new netifs and calls init/register steps again, but it does not rebuild a clean baseline after a real shutdown.
- **Concrete case:** On a real device, call `wifi_init()` successfully, then `wifi_shutdown()`, then `wifi_init()` again (the header promises this is allowed and `test/test_wifi.c:280-285` expects it). The first event group is leaked, event handlers remain registered, and the second init recreates netifs / re-runs WiFi initialization on top of objects that were never fully torn down, so reinit can fail or leak resources instead of restoring a working WiFi subsystem.
- **Fix:** Make shutdown release everything init owns: store handler instances, unregister them, delete the event group, and deinit/destroy WiFi and netif resources in the right order (or explicitly choose and document a one-time-init design and remove the reinit contract). Keep `wifi_init()` able to recover cleanly after a real shutdown.
- **If we skip it:** WiFi restart/reinit paths leak handles on hardware and can stop working after the first shutdown.

### W2 — Must fix: `wifi_init` leaves the component stuck after a mid-init failure

- **Location:** `components/wifi/src/wifi.c:57-123`
- **What this is:** `wifi_init()` sets `s_event_group` near the start and treats a non-NULL event group as "already initialized". Later steps (`esp_netif_init`, event loop creation, `esp_wifi_init`, handler registration, mode/config/start/connect) use `ESP_RETURN_ON_ERROR` or return on failure.
- **Problem:** If any step fails after `s_event_group` is created, the function returns an error without cleaning up what was already created and without clearing `s_event_group`. A later `wifi_init()` call immediately returns `ESP_ERR_INVALID_STATE`, and `wifi_shutdown()` sees the non-NULL event group but cannot repair the half-initialized ESP-IDF state.
- **Concrete case:** `esp_wifi_set_mode`, `esp_wifi_start`, or `esp_wifi_connect` fails during first boot (driver/radio/config error) → `wifi_init()` returns an error → `s_event_group` is still set and some WiFi/netif objects may already exist → retrying `wifi_init()` returns `ESP_ERR_INVALID_STATE` and WiFi cannot be brought up without a reboot.
- **Fix:** On any init failure, roll back completed steps and clear module state so callers can retry. A small `goto fail`/cleanup path that undoes only the resources created so far is enough.
- **If we skip it:** A temporary startup failure can leave WiFi permanently unavailable until reboot, with no clean recovery path.

### W3 — Should fix: `wifi scan` can fail in AP-only startup mode

- **Location:** `components/wifi/src/wifi.c:105-113`, `components/wifi/src/wifi.c:175-194`, `components/wifi/src/wifi_cmd.c:125-143`
- **What this is:** When no STA credentials are stored, `wifi_init()` starts the device in `WIFI_MODE_AP` only. The `wifi scan` console command calls `wifi_scan()`, which runs `esp_wifi_scan_start(...)` directly.
- **Problem:** WiFi scanning normally needs the STA interface to be started. In AP-only mode a scan request can fail, so a user who boots with no saved network and runs `wifi scan` to find one may get a scan error instead of a network list.
- **Concrete case:** Boot with empty NVS credentials → component starts in AP-only mode (`main` calls `wifi_init()` at startup) → user runs `wifi scan` from the console → `esp_wifi_scan_start` returns an error → `wifi_cmd` prints `wifi: scan failed (...)` and no networks are shown.
- **Fix:** Before scanning, ensure STA is available for the scan (for example temporarily enable `APSTA` when in AP-only mode, then restore the previous mode/config if needed), or return a clear message that a scan requires STA/connect mode. Pick the smallest behavior that matches how users are expected to find and join a network.
- **If we skip it:** First-time setup can be confusing: the device creates an AP but cannot list nearby networks to connect to.

## Validation / not checked

- Full ESP-IDF device build and on-device WiFi behavior (ESP-IDF toolchain/hardware not exercised in this review).
- `test/test_wifi.c` and related host/unit tests after fixes (run relevant checks per `AGENTS.md` when implementing).
- Concurrent use of WiFi APIs/console commands from multiple tasks; review assumed single-user, occasional use.
- Interaction details with `components/network` beyond the direct API calls observed (for example scan behavior while HTTP/ping workers are active).

## Related code/tests

- Implementation: `components/wifi/src/wifi.c`, `components/wifi/src/wifi_cmd.c`, `components/wifi/src/wifi_default_config.h`, `components/wifi/include/wifi.h`
- Build: `components/wifi/CMakeLists.txt`
- Existing host tests: `test/test_wifi.c`
- Callers/context: `main/main.c:96`, `components/network/src/network_status.c`, `components/berry_lang/src/berry_network.cpp`, `storage/ts/scripts/wifi.ts`

## Lean

- `Lean: -0 lines, -0 dependencies possible.` (no clear safe deletions or dependency removals identified without changing behavior/contracts.)
