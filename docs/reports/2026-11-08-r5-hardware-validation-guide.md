# R5 Hardware Validation Guide — Berry Memory Instrumentation & Lifecycle

**Date:** 2026-11-08  
**Scope:** `docs/2026-11-08-consolidated-improvement-plan.md` R5 / `docs/2026-11-02-memory-consumption-improvements.md` Phase 1  
**Why hardware is required:** R5 adds ESP32 heap/watermark/fragmentation reporting, Berry GC lifecycle checkpoints, app lifecycle cleanup behavior, and GC step-rate experiments. These depend on the real ESP32 heap, display/UI/VFS stack, and pinned Berry VM on target hardware. Host unit tests cover only the source-buffer ownership helper and shell/mock wiring.

## Prerequisites

- Supported COS hardware (CYD/ESP32 target used by this repo) with current firmware built from this workspace.
- Serial console access (USB) at the firmware's configured baud rate.
- Flash + SD data deployed as needed (`storage/flash_data/...` scripts available on `/flash`, especially `tests/r5/memory_churn.be`; test apps like `apps/tap_dash.be`, `apps/breakout.be` available).
- A way to capture serial logs to a file (recommended): `idf.py monitor`, `putty` logging, `tio`/`screen` with logging, or equivalent.
- Build includes the R5 changes:
  - `[R5]` lifecycle checkpoint logs in `berry_runtime`
  - `berry profile` shows `step_rate` and heap/internal snapshot
  - `berry gc-step <rate>` command (default 200)

## Safety / baseline notes

- **Do not leave a non-default GC step rate in production use.** Default is 200. After experiments, run `berry gc-step 200`.
- GC step-rate experiments (200/150/125) are for measurement only until benchmarks justify a default change.
- Run network/Bluetooth-heavy cases separately from pure Berry memory cases if heap is already constrained (R2 found TLS/network can collapse heap in some paths).

## 1. Lifecycle checkpoint log verification

**Goal:** Confirm every required lifecycle checkpoint emits Berry GC + heap metrics.

### Steps

1. Boot the device and open the serial monitor.
2. Run:
   ```text
   berry profile
   berry run /flash/apps/tap_dash.be
   ```
3. Interact briefly if needed, then exit/close the app:
   ```text
   berry close
   ```
   (Or use the app's normal exit mechanism; `ui.exit()`/ESC behavior depends on the app.)
4. Capture log output around each phase.

### Expected `[R5]` checkpoints

Look for lines containing `[R5]` with fields:
`berry_gc=... heap_free=... heap_min=... heap_largest=... internal_free=... internal_min=... internal_largest=...`

| Checkpoint | Typical phase string | When |
|---|---|---|
| 1 | `1 Berry VM initialized` | First Berry VM init (may happen at boot/autoexec or first `berry` command depending on flow) |
| 2 | `2 Berry source loaded` | After app/script source is read into RAM |
| 3 | `3 Berry compilation complete (source buffer released)` | After `be_loadbuffer`/compile; owned source buffer released |
| 4 | `4 Berry app constructed` | After class instance construction, before setup |
| 5 | `5 Berry app setup complete` | After `setup()` and UI start succeed |
| 6 | `6 Before app teardown cleanup` or `6 Before app-state cleanup...` | Before teardown/UI stop/resource cleanup |
| 7 | `[R5] 7 GC complete | ...` (includes `reclaimed`/`from`/heap metrics) | After `be_gc_collect()` during app cleanup paths |

### Pass criteria

- Checkpoints 1–7 are observed across at least one full app open/close cycle (checkpoint 1 may appear only once per VM lifetime).
- Each checkpoint reports all required fields (Berry GC managed bytes, free/default heap, minimum watermark, largest default block, internal free, internal min, internal largest).
- No crash, watchdog reset, or allocation failure during the cycle.

### Record

Save serial log excerpts with timestamps and note the app used.

## 2. Source buffer release verification

**Goal:** Confirm source buffers are not retained after compilation, including failures.

### Steps

1. Check normal compile path:
   ```text
   berry profile
   berry run /flash/apps/tap_dash.be
   berry close
   berry profile
   ```
   Observe checkpoint 3 appears after source load (checkpoint 2). A large drop in transient pressure is not directly printed, but checkpoint 3 is emitted only after `compile_and_release_source` returns (buffer released).
2. Check compile-failure path:
   - Create a small invalid Berry file on `/flash` or `/sd`, e.g. `/flash/tests/r5/bad.be` containing:
     ```berry
     def oops(
     ```
   - Run:
     ```text
     berry profile
     berry run /flash/tests/r5/bad.be
     berry profile
     ```
3. Check missing file path:
   ```text
   berry run /flash/tests/r5/does_not_exist.be
   berry profile
   ```

### Pass criteria

- Valid scripts/apps still run.
- Invalid script reports a Berry/compile error and returns control to the shell; no hang.
- Missing file reports file/path error.
- Repeated compile failures do not progressively reduce `heap_min`/`internal_min` or largest blocks beyond normal noise; `berry gc` recovers managed Berry memory as expected.
- No double-free/panic/reset.

## 3. Repeated app start/stop regression (leaks/fragmentation)

**Goal:** Detect progressive memory loss or fragmentation across many app lifecycles.

### Procedure A — manual loop (recommended)

Use a representative UI app (e.g. `tap_dash.be`, `breakout.be`). For each app, run at least **10 cycles**; **25 cycles** preferred.

1. Baseline:
   ```text
   berry profile
   ```
   Record: `heap_free`, `heap_min`, `heap_largest`, `internal_free`, `internal_min`, `internal_largest`, Berry `managed`, `step_rate`.
2. Cycle:
   ```text
   berry run /flash/apps/tap_dash.be
   ```
   Exit the app normally (UI exit/ESC path supported by the app), or run `berry close` if needed.
   ```text
   berry profile
   ```
3. Every 5 cycles, also run:
   ```text
   berry gc
   berry profile
   ```
4. Repeat for the second app.

### Procedure B — churn probe (non-UI allocation pressure)

```text
berry profile
berry run /flash/tests/r5/memory_churn.be
berry profile
berry gc
berry profile
```
Repeat 5–10 times.

### Pass criteria

- Berry `managed` bytes return near a stable baseline after GC (small variation acceptable).
- `heap_min` / `internal_min` may drop during first uses but should stabilize; they must not show a steady per-cycle decline after warm-up.
- `heap_largest` / `internal_largest` should remain sufficient for normal operations; a severe monotonic decline indicates fragmentation/leak risk.
- No failure to open app after repeated cycles (unless caused by unrelated known constraints).

### Investigate if

- Managed bytes grow after every GC.
- Min heap drops every cycle.
- Largest block shrinks until allocation failures occur.
- UI callbacks/timers appear to fire after app close (check logs/behavior).

## 4. GC step-rate experiments (benchmark only; default unchanged)

**Goal:** Measure 200 vs 150 vs 125 before considering any production default change.

### Cases

For each step rate in `{200, 150, 125}`:

1. Set rate:
   ```text
   berry gc-step 200   (or 150 / 125)
   berry profile
   ```
2. Run the same workload set:
   - open/close `tap_dash.be` 5 times
   - run `berry run /flash/tests/r5/memory_churn.be` 3 times
   - if time permits, run a second UI app 5 open/close cycles
3. After each workload block:
   ```text
   berry profile
   berry gc
   berry profile
   ```
4. Restore default:
   ```text
   berry gc-step 200
   ```

### Metrics to record

| Metric | How |
|---|---|
| Peak memory pressure / min available heap | `heap_min`, `internal_min` from `berry profile` and `[R5]` checkpoints |
| Largest block / fragmentation | `heap_largest`, `internal_largest` |
| Berry managed memory after GC | `berry gc` output and `managed` in profile |
| GC frequency/effect | count `Berry GC` / checkpoint 7 logs during workload; reclaimed bytes |
| Responsiveness / frame impact | qualitative observation: UI lag, missed input, stutter during app run |

### Pass criteria

- Measurements recorded for 200, 150, and 125.
- Default remains 200 in code/docs unless evidence shows a clear, safe improvement.
- Any chosen non-default rate is justified by data (lower peak/min-heap pressure without unacceptable responsiveness/frame impact).

## 5. Host-side Berry compile validation (developer/CI workstation)

Not hardware, but required by R5:

```powershell
# Uses build/berry-host-cos/bin/berry.exe if present, else BERRY_EXE/PATH candidates.
powershell -ExecutionPolicy Bypass -File .\scripts\validate-berry-scripts.ps1
```

A COS-configured host compiler is recommended for strict parsing parity; the script prepends COS global declarations to avoid false undeclared-global errors from stock configs. Document the compiler path/config used in CI.

## Reporting template

```text
R5 hardware validation
Date:
Firmware commit/build:
Device:
Serial log: attached/path

1. Lifecycle checkpoints (1-7): PASS/FAIL
   Notes:

2. Source buffer release / compile failures: PASS/FAIL
   Notes:

3. Repeated start/stop (app, cycles): PASS/FAIL
   tap_dash cycles=  heap_min trend=  internal_min trend=  largest trend=  managed after GC=
   breakout cycles=  ...
   churn rounds=

4. GC step-rate benchmarks: PASS/FAIL (data collected)
   200: heap_min= internal_min= largest= responsiveness=
   150: heap_min= internal_min= largest= responsiveness=
   125: heap_min= internal_min= largest= responsiveness=
   Recommendation: keep 200 / change to ...

Issues/follow-ups:
```

## Known limitations / follow-ups

- `berry profile`/`get_memory_info` report the configured GC step rate tracked by COS runtime (default 200, updated by `berry gc-step`); if Berry internals change, verify parity.
- Host Berry compile validation uses pinned Berry source but host config is not identical to ESP32 firmware unless a COS-configured host compiler is built (`scripts/berry_conf.host_validation.h`). The validation script adds COS global declarations to reduce false positives.
- Network/TLS stress during memory tests can obscure Berry-specific results (see R2/R8). Prefer isolated Berry/UI tests first.
