# R5 Hardware Results — 2026-11-08

**Source:** User-provided serial console output (COS target). R5-HW2 and R5-HW3 completed 2026-11-08. Default GC step restored to **200**.

## 1. Berry churn probe (`/flash/tests/r5/memory_churn.be`)

| Step | System/internal (system_get_info lines) | Heap snapshot (R5) | Berry GC managed | Notes |
|---|---|---|---|---|
| Baseline `berry profile` | free=25660 min=20072 largest=19456 | free=25660 min=20072 largest=19456 / internal_free=48856 internal_min=43236 internal_largest=22528 | 3883, step 200 | HTTP worker not started (cfg=6144) |
| After `memory_churn.be` | free=25816 min=20072 largest=19456 | free=25816 min=20072 largest=19456 / internal 49012/43236/22528 | 3883 | Stable managed bytes; no leak observed |
| After `berry gc` | free=25816 min=18516 largest=19456 | free=25816 min=18516 largest=19456 / internal 49012/41680/22528 | 3883, reclaimed 0 | GC watermark/min changed; managed bytes stable |

**Assessment:** Non-UI churn probe shows stable Berry managed memory and no progressive leak. GC reclaimed 0 (nothing collectible) — expected if churn objects were already collected or retained as expected.

## 2. Breakout repeated open/close (`berry run /flash/apps/breakout.be`, `berry close`, 4 cycles)

| Cycle | free | min | largest | internal_free | internal_min | internal_largest | Berry managed | Notes |
|---:|---:|---:|---:|---:|---:|---:|---:|---|
| After cycle 1 | 15400 | 6420 | 5376 | 38596 | 29584 | 22528 | 4382 | Large initial warm-up/drop from churn baseline |
| After cycle 2 | 14916 | 6420 | 3328 | 38112 | 29584 | 22528 | 4358 | Managed/free small change; largest more fragmented |
| After cycle 3 | 14920 | 6420 | 5632 | 38116 | 29584 | 22528 | 4358 | Stable; largest recovered |
| After cycle 4 | 14920 | 6420 | 5632 | 38116 | 29584 | 22528 | 4358 | Stable across cycle 3→4 |

**Assessment:** After the first open/close warm-up, cycles 2–4 show **stable** `free`, `min` watermark, `internal_min`, `internal_largest`, and Berry `managed` bytes. Largest default block fluctuated (3328→5632) but stabilized; no monotonic per-cycle leak trend observed in this sample.

**Caveats / follow-ups:**
- Only 4 cycles captured; guide recommends **10–25 cycles** and a second representative app for stronger confidence.
- Initial delta after first breakout run is large (~10.4 KB free, largest 19456→5376, min watermark 6420). Determine whether this is expected persistent UI/display/app runtime state, fragmentation, or reclaimable leakage.
- Lifecycle `[R5]` serial checkpoint lines were not included in the pasted output; verify checkpoints 1–7 are emitted during open/close as per the guide.
- `System heap` and `Internal heap` lines printed by `berry profile` appear identical in this output, while the R5 `Heap snapshot` line shows distinct internal metrics. This may be a `system_get_info` reporting issue (pre-existing/R2) and should be checked separately; R5 snapshot fields look plausible.

## 3. Post-restart fragmentation / `ESP_ERR_NO_MEM` on second Breakout run (2026-11-08)

**Scenario (after ESP restart):**
1. `berry run apps/breakout.be` (ok)
2. `berry close`
3. `berry profile` reported: `free=20396 min=7264 largest=4608` (default caps), `internal_free=43592 internal_min=30428 internal_largest=22528`, Berry `managed=4382`.
4. `berry run apps/breakout.be` failed: `berry: execution failed (ESP_ERR_NO_MEM)`.

**Assessment:** This looks like **heap fragmentation / insufficient contiguous default-capability block** (largest 4608 bytes) rather than Berry GC-managed leak (managed only ~4.3 KB). It may occur during source buffer allocation, compilation objects, app construction, or UI/startup allocation — the console message alone is not enough to identify the failing stage.

**Diagnostic captured (info-level serial log, 2026-11-08):**
```text
COS/flash> berry run apps/breakout.be
I (70598) berry: [R5] Before Berry file load | berry_gc=4382 heap_free=30396 heap_min=7264 heap_largest=8192 internal_free=53592 internal_min=30428 internal_largest=22528
W (70598) berry: Could not allocate Berry source buffer (8224 bytes); free=30396, largest=8192, internal_largest=22528
berry: execution failed (ESP_ERR_NO_MEM)
```

**Conclusion:** Failure is at **source buffer allocation** before compile/load. `apps/breakout.be` is 8223 bytes on disk, so runtime requests `8224` bytes (`length+1`, NUL-terminated). Default heap has `30396` free but largest contiguous block is only `8192` (< 8224). Internal heap had `internal_largest=22528` at this point, but `read_virtual_file()` currently uses default `calloc`.

**Code follow-ups applied (2026-11-08, hardware revalidation pending):** `components/berry_lang/src/berry_runtime.cpp`:
1. `read_virtual_file()`: `vfs_stat()` → size, allocate short-lived source buffer, then `vfs_read_file()`. Robust allocator (freed after `be_loadbuffer()`): checks post-stat `def_largest`/`int_largest`; if default contiguous is insufficient and internal fits, tries `MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT` first; then `malloc`, `MALLOC_CAP_8BIT`, `MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT`. Emits post-stat warning on failure. Rationale: Breakout 8224-byte load failed when default heap fragmented (4352/6400) while `internal_largest` remained 22528; also `vfs_stat` fragmented heap after pre-load checkpoint.
2. `collect_berry_garbage()`: consolidated to a single `[R5] 7 GC complete | ...` log (includes reclaimed bytes + heap metrics) to reduce stdio pressure in low-heap/fragmented states. This does not fix underlying heap/stdio corruption (see panic follow-up).

## 5. Heap corruption panics in stdio during R5 logging (info logging, 2026-11-08)

**Symptom:** After Breakout run/close in a fragmented state (`free=19252, largest=4352, min=3412`), `berry profile` / `berry gc` worked, then a subsequent `berry profile` produced garbled console output and the device panicked.

**Panic:**
```text
Guru Meditation Error: Core 0 panic'ed (LoadStoreError). Exception was unhandled.
EXCVADDR: 0x40000000 (ROM)  EXCCAUSE: 0x00000003
Backtrace: ... fwrite -> stdout_write_hook (text_console.cpp:210) -> fopencookie/fun_write -> __bufio_flush_locked/__bufio_put -> vfprintf/vprintf -> esp_log -> collect_berry_garbage() (berry_runtime.cpp:119) -> close_active_app -> execute_app_source -> berry_runtime_run_file -> cmd_berry -> esp_console_run -> shell input...
```

**Analysis:** Crash is in **picolibc stdio/fwrite while flushing an ESP log** (`collect_berry_garbage()` R5 checkpoint 7 log), with invalid access to `0x40000000`. This indicates **heap/stdio control-structure corruption** (FILE/buffer/pointers corrupted), not a simple allocation failure. R5 logging is the trigger (more stdio activity), but the corruption likely originates from severe heap pressure/fragmentation (`largest=4352`) and/or an existing overflow/use-after-free in the UI/text_console/display/Berry/stdio path. Needs targeted debugging.

**Second panic (new firmware, 2026-11-08):** same `LoadStoreError`/`EXCVADDR 0x40000000` in `fwrite` → `stdout_write_hook (text_console.cpp:210)` → `esp_log`, but triggered from `log_lifecycle_checkpoint` at `berry_runtime.cpp:94` via `berry_runtime_run_file` (early run path). Same root signature: stdio/heap corruption surfaced by formatted logging.

**Mitigation applied (R5, best-effort, reflash pending):** `log_lifecycle_checkpoint()` and the consolidated GC checkpoint log skip formatted `ESP_LOGI` when `heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT) < 1536`. This reduces stdio pressure in critically fragmented states but **does not fix heap corruption**. In those states, verbose R5 checkpoints may be omitted; use `berry profile`/targeted captures when heap is healthier, or `logging error` for stability testing.

**Debug follow-ups (R5-FU1, critical):**
- Reproduce with `logging error` to reduce stdio pressure; check if panic still occurs (if it does, corruption is independent of R5 info logs).
- Enable heap debugging in `sdkconfig` (measure overhead first): `CONFIG_HEAP_POISONING_LIGHT` or `COMPREHENSIVE`, task tracking/stack checks as appropriate.
- Inspect `text_console.cpp:210` stdout hook and stdio buffer ownership/lifetime under low-memory and fragmented conditions.
- Consider routing essential diagnostics through a lower-allocation/non-stdio path if needed, but prioritize finding the corruption source.

**Remaining follow-ups (measure on hardware):**
- Reflash latest source-buffer fallback fix and retest post-close Breakout reload with `logging info` (capture `[R5]` checkpoints through the failing stage).
- Confirm full `[R5]` checkpoints 1–7, 10–25 restart cycles + second app, and GC step-rate benchmarks (200/150/125).
- Investigate first-cycle/post-close fragmentation and the stdio/heap corruption panic under low-heap conditions.

## 4. Additional hardware evidence (2026-11-08, latest firmware)

**Checkpoint 1 (clean boot):** ✅
```text
I (2298) berry: [R5] 1 Berry VM initialized | berry_gc=3807 heap_free=55984 heap_min=54628 heap_largest=53248 internal_free=79180 internal_min=77792 internal_largest=53248
```

**Tap Dash (R5-HW2, 10 cycles):** ✅ Checkpoints 2,6,7,3,4,5 and close 6,7 observed across runs (2542-byte `apps/tap_dash.be`). Post-close trend:

| Cycle | Berry managed | reclaimed | heap_free | heap_largest | heap_min | internal_free | internal_min | internal_largest | Notes |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 1 | 4374 | 5256 | 19024 | 9216 | 2920 | 42220 | 26084 | 22528 | warm-up |
| 2 | 4342 | 5320 | 19040 | 9216 | 2920 | 42252 | 26084 | 22528 | settling |
| 3–7 | 4342 | 5320 | 19056 | 9216 | 2920 | 42252 | 26084 | 22528 | steady state |
| 8 | 4342 | 5348 | 29056 | 9216 | 2920 | 52252 | 26084 | 22528 | BT HID auto-connect failed during run (`BLE_HIDH OPEN 0x85`), heap recovered after close |
| 9–10 | 4342 | 5320 | 29056 | 9216 | 2920 | 52252 | 26084 | 22528 | steady state |

**Assessment:** After warm-up, Berry managed memory returns to **4342** every cycle with consistent reclaim (**5320**). `heap_largest` and all watermarks stable (`heap_min=2920`, `internal_min=26084`, `internal_largest=22528`). **No progressive memory loss observed.** Cycle 8 `heap_free` increase is attributable to unrelated BT HID activity settling, not a Berry leak. `W (text_console) Scrollback allocation failed` interleaving observed under fragmentation but no R5-FU1 panic.

**Breakout reload/restart stability:** Breakout loaded successfully in repeated runs (8223 bytes, checkpoints 2,6,7,3,4,5 and close 6,7). In a 7-cycle stable sample, post-close state was identical after warm-up: `berry_gc=4609`, `reclaimed=10555` (10609 first), `heap_free=28560`, `heap_largest=8704`, `internal_free=51756`, `internal_largest=22528`; watermarks stable (`heap_min=1256`, `internal_min=24420`). **No progressive memory loss observed** in this sample. Earlier worst-case `default largest < 8224` reload failure was diagnosed and fixed in code; that exact fragmented state was not re-triggered in the latest stable sample.

**Transient anomaly:** one `berry run /flash/apps/breakout.be` returned `ESP_FAIL` after `[R5] Before Berry file load` with no checkpoint 2/NO_MEM warning (output interleaved by scrollback warnings). Retry succeeded; capture a clean serial trace if reproducible.

**GC step-rate (R5-HW3, 2026-11-08):** ✅ Benchmarked with **identical workload** per rate: Breakout run/close ×5, then `/flash/tests/r5/memory_churn.be` ×3. *Note:* on-device `memory_churn.be` loaded as **0 bytes** in all three runs (deployment/content issue on target), so the churn portion exercised an empty script (GC `reclaimed=256, after=4551` each time). Breakout ×5 comparison is valid and consistent across rates. No responsiveness/frame impact noticed by tester.

| Rate | Workload (identical) | Post-close Breakout steady state (cycles 2–5; cycle 1 first) | berry_gc after close | reclaimed (close GC) | heap_free (post-close) | heap_largest | heap_min | internal_min | internal_largest | Final managed/free/largest | Responsiveness |
|---:|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| **200** (default) | breakout ×5 + churn ×3 (0 B on device) | 4551, reclaimed 10555 (10581 first), stable | 4551 | 10555–10581 | 28400 | 8704 | 2920 | 26084 | 22528 | 4551 / 28400 / 8704 | no impact noticed |
| **150** | same | 4551, reclaimed 10555, stable all cycles | 4551 | 10555 | 28400 | 8704 | 2920 | 26084 | 22528 | 4551 / 28400 / 8704 | no impact noticed |
| **125** | same | 4551, reclaimed 10555 (10581 first), stable | 4551 | 10555–10581 | 28400 | 8704 | 2920 | 26084 | 22528 | 4551 / 28400 / 8704 | no impact noticed |

**Conclusion:** No measurable difference in post-close managed memory, heap/largest, watermarks, or observed responsiveness across 200/150/125 for this workload. **Default remains 200.** `berry gc-step 200` restored and confirmed (`step_rate=200`, `managed=4551`, `free=28400`, `largest=8704`). ✅

**Failure paths (guide §2):** missing file → `ESP_ERR_NOT_FOUND` (clean). Compile error (`/flash/tests/r5/bad.be`, `def oops(`) → checkpoint 2,3 logged (source released), Berry syntax error reported, checkpoint 7 GC, `ESP_FAIL`. 0-byte file handled cleanly. ✅

## Overall R5 status — ✅ Accepted (2026-11-08), follow-ups tracked

- **Implemented and host-validated:** lifecycle checkpoint instrumentation (1–7), extended profile/memory API, GC step-rate command/API (default **200**), source-buffer ownership/release + fragmented-heap load fallback, host `.be` compile validation (22/22), churn/restart assets, hardware guide.
- **Hardware evidence collected (acceptance satisfied):**
  - **Checkpoint 1** (clean boot) ✅.
  - **Checkpoints 2–7 / teardown 6–7 / GC reclaimed** observed for Tap Dash and Breakout ✅.
  - **R5-HW2 restart-loop regression (two apps)** ✅: Tap Dash 10 cycles and Breakout 7 cycles — stable post-warm-up Berry managed memory, stable watermarks/largest blocks, **no progressive memory loss** observed.
  - **Source-buffer lifecycle/reload** ✅: 8223-byte Breakout loads succeed; source-buffer `ESP_ERR_NO_MEM` was diagnosed and fixed in code (fallback allocator); release after `be_loadbuffer()` verified by checkpoints/tests.
  - **Failure paths** (guide §2) ✅: missing file, compile error, 0-byte file handled cleanly.
  - **R5-HW3 GC step-rate benchmarks 200/150/125** ✅ (identical workload, no measurable difference, no responsiveness impact noticed). **Default unchanged (200) restored/confirmed.**
- **Open follow-ups (not blocking R5 acceptance):**
  - **R5-FU1 (critical):** heap/stdio corruption panic root cause unresolved (`LoadStoreError`/`EXCVADDR 0x40000000` in `fwrite` → `stdout_write_hook` `text_console.cpp:210` during logging). Not reproduced in latest capture. Reproduce with `logging error`; enable heap poisoning/stack checks; audit stdout hook/fopencookie/buffer ownership.
  - **R5-FU2:** `berry profile` `System heap`/`Internal heap` lines from `system_get_info` appeared identical while R5 snapshot shows distinct internal metrics (likely pre-existing display/reporting issue).
  - **Deployment note:** on-device `/flash/tests/r5/memory_churn.be` was 0 bytes during HW3; repo asset exists — re-deploy if churn benchmark should be rerun meaningfully.
  - **Optional:** worst-case `default largest < 8224` reload revalidation if reproducible.
