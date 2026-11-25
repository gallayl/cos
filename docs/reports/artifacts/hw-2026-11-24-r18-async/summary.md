# R18 on-device validation attempt (COM3)

- **Date:** 2026-11-24
- **Port/baud:** COM3 / 115200
- **Firmware:** `fc7cbe1` (branch `feat/r18-async-await-continuation`, build shown in boot log: `App version: fc7cbe1`, `Compile time: Nov 24 2026 16:13:37`, ESP-IDF v6.1)
- **Tool:** `python scripts/hw_console_session.py` (pyserial 3.5)
- **Artifacts:** `docs/reports/artifacts/hw-2026-11-24-r18-async/` (probe.log, probe2.log, r18-validate.log, sleep-return.log, debug-sleep.log, type-sleep.log, meminfo.log, r18-workaround.log, close-retry.log, post-reset.log, command files, this summary)

## Scope note
This PR changes frontend Berry compiler only (async try/catch lowering). Firmware C code is unchanged from origin/main at same base; on-device validation here targets R18 runtime foundation already present in firmware. Findings below are runtime/firmware observations, not caused by this PR's TS compiler diff.

## Results

| Step | Expected | Actual | Outcome |
|---|---|---|---|
| Probe / device identity | Device responds on COM3, R18 runtime present | Booted, `App version: fc7cbe1`. `berry eval "import system; print(type(system.sleep))"` => `function`. `berry eval "print(type(Promise))"` caused **Guru Meditation Error: Core 1 panic'ed (LoadProhibited)**, device rebooted (probe.log). | **FAIL/INCONCLUSIVE** (crash; low heap at time of crash) |
| Post-reboot probe | Device healthy after reboot | `mem`/`berry profile`/`berry app` responded; heap low (free ~13k, min 8708) after full services boot. | PASS (basic console) |
| `system.sleep` returns Promise | `system.sleep(200)` returns Promise; callback fires after delay | `system.sleep(200)` returned **int `200`** and logged `Async timer scheduling failed: ASYNC_TIMER_LIMIT`; then `.then(...)` failed with `'int' value has no method 'then'` (r18-validate.log, type-sleep.log, sleep-return.log). Direct `_cos_system_sleep(200)` returned `instance <Promise()>` (debug-sleep.log). | **FAIL** - native `system.sleep` wrapper returns ms int instead of Promise (pre-existing firmware bug; reproduced on fc7cbe1) |
| Scheduler pump / sleep callback | R18_SLEEP_OK printed after 300ms | With `_cos_system_sleep` workaround: `Async timer scheduling failed: ASYNC_TIMER_LIMIT` immediately; no callback printed (r18-workaround.log, post-reset.log). | **FAIL** - async timer scheduling rejected (ASYNC_TIMER_LIMIT) on clean boot and after `berry close` |
| Teardown / berry close | `berry close` cleans async state; heap recovers | `berry close` executed; heap/GC changed but subsequent `_cos_system_sleep` still hit ASYNC_TIMER_LIMIT (close-retry.log, post-reset.log). | **INCONCLUSIVE/FAIL** - close ran but timer limit persisted |
| R5-style heap/watermark/teardown evidence | Repeated run/close/VM reset, no leaks/dangling callbacks | Not completed due to ASYNC_TIMER_LIMIT blocking async sleep and very low heap on full-featured build. | **NOT COMPLETED** |
| Limits/overflow behavior | Deterministic overflow/fail-fast | ASYNC_TIMER_LIMIT observed (logged). Deterministic Promise rejection path not fully validated because system.sleep wrapper bug masked it; _cos_system_sleep path logged limit but callback not observed in captured window. | **PARTIAL** |
| No regressions to existing Berry/TS runtime | Basic berry eval/mem/profile work | Basic console commands worked; `mem` command intermittently returned `ESP_ERR_NOT_FOUND` under low memory. | **INCONCLUSIVE** (low-memory environment) |

## Key findings / blockers
1. **Pre-existing firmware bug (not in this PR diff):** `system.sleep(ms)` native wrapper returns `ms` (int) instead of Promise. Direct `_cos_system_sleep(ms)` returns Promise. This breaks `system.sleep(...).then(...)` and likely breaks async_demo on device.
2. **ASYNC_TIMER_LIMIT on clean boot:** `_cos_system_sleep`/sleep attempts immediately hit `Async timer scheduling failed: ASYNC_TIMER_LIMIT`, even after reset and after `berry close`. Async callbacks did not fire in captured windows. Root cause not isolated in this session (could be timer leak, slot accounting bug, or memory-pressure interaction).
3. **Low-memory full build:** full-service firmware left very little free heap (min observed down to 512/752 bytes), causing instability (`mem: ESP_ERR_NOT_FOUND`, one Guru Meditation crash during probe). Reliable R18 timing/teardown/leak validation needs a controlled environment/build profile.
4. **Hardware validation not passed.** R18 spec hardware/manual checks remain unverified/failed in this session. Do not treat as resolved without evidence.

## Recommended next actions
- Fix `native_system_sleep` to return the Promise result of `_cos_system_sleep` (check Berry C API return/stack convention; add unit/integration coverage if feasible).
- Investigate ASYNC_TIMER_LIMIT on boot/after close (timer slot leak/accounting, teardown completeness, init state).
- Re-run R18 hardware validation on controlled build/profile with sufficient heap; capture R5-style heap/watermarks across repeated run/close/VM reset.