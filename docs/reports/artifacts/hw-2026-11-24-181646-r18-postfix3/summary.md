# HW Validation - R18 postfix3 reflash (COM3, a40ed17 flashed)

- Date: 2026-11-24
- Port: COM3, 115200, `--reset`
- Firmware: `a40ed17` (Compile Nov 24 2026 18:14:30, ESP-IDF v6.1)
- Artifacts: redacted logs in this directory.

## Results
- Boot OK. `ui: Starting UI task (background, ...)` observed — `ui::ensure_task()` fix active.
- `type(Promise)=class` — PASS.
- `berry profile` shows `Async: timers=0 microtasks=0` at clean boot and after drains — PASS (pump draining observed).
- `cos_async_sleep` returned `ok:true` when queue empty — PASS.
- `system.sleep(300/500)` returned `int` with `ASYNC_TIMER_LIMIT` during several attempts; async timer queue observed filling to limit within ~20s and then draining to 0 later. Callback not observed. — **FAIL/INCONCLUSIVE**.
- Root cause of fill-up not isolated on device (no Berry autorun script loaded; no app running). Could be background workload + low heap or an async scheduling loop; needs investigation.

## Status
- **NOT PASSED / blocking**: core fixes present (ensure_task, return value, async reset, profile counters) but reliable on-device system.sleep/callback proof not obtained on this full-service build. Needs targeted investigation/revalidation.