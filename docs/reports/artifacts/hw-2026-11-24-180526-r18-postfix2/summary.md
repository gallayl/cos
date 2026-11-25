# HW Validation - R18 postfix2 reflash (COM3, ab43de7 flashed)

- Date: 2026-11-24
- Port: COM3, 115200, `--reset`
- Firmware: `ab43de7` (Compile Nov 24 2026 18:02:42, ESP-IDF v6.1)
- Artifacts: redacted logs in this directory.

## Results (on flashed ab43de7)
- Boot OK, app version/compile time confirmed.
- `type(system.sleep)=function`, `type(Promise)=class` — PASS.
- `berry profile` shows `Async: timers=0 microtasks=0` at clean boot — PASS (diagnostic field present).
- `system.sleep` / `cos_async_sleep` callbacks **did not fire** in shell/no-app mode; timers accumulated to ASYNC_TIMER_LIMIT over time — **FAIL**.
- Root cause: `ui::start()` (which creates UI task) is only called on Berry app launch; with no active app, UI task never started, so tick_callback/async pump never ran despite ui_active-false branch.

## Fix in code (not yet flashed)
- Added `ui::ensure_task()` and call from `berry_runtime_init` to start background UI task; combined tick pumps when ui_active is false.

## Status
- **NOT PASSED / blocking**: requires reflash with ensure_task fix and re-validation on device.
