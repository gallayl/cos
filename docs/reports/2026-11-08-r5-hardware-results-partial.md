# R5 Hardware Results (Partial) — 2026-11-08

**Source:** User-provided serial console output (COS target). GC step-rate benchmarks still pending.

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

**Code follow-up applied (2026-11-08, hardware revalidation pending):** `components/berry_lang/src/berry_runtime.cpp` `read_virtual_file()`:
1. `vfs_stat()` → determine size, then allocate the short-lived source buffer, then `vfs_read_file()` (buffer secured before read-path allocations).
2. Robust allocator for the short-lived buffer (freed after `be_loadbuffer()`): checks post-stat `def_largest`/`int_largest`; if default contiguous is insufficient and internal fits, tries `MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT` first; then `malloc`, `MALLOC_CAP_8BIT`, `MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT`. Emits a specific warning with post-stat sizes on failure. Rationale: `[R5]` logs showed checkpoint `largest=18432` before load but allocation failed after `vfs_stat`-era fragmentation with `default largest 4352` while `internal_largest 22528`.

**Remaining follow-ups (measure on hardware):**
- Reflash and retest both cases: (a) info logging, clean state/load; (b) error logging, first Breakout run + `berry close` + second `berry run apps/breakout.be`.
- Capture `[R5] Before Berry file load`, allocation warnings (if any), and `berry profile` before/after.
- If `ESP_ERR_NO_MEM` persists, capture `heap_free/heap_largest/internal_largest` and requested bytes; evaluate next measured fallback/capability strategy.
- Confirm full `[R5]` checkpoints 1–7, 10–25 restart cycles + second app, and GC step-rate benchmarks (200/150/125).

## 4. GC step-rate benchmarks

**Pending** (user indicated benchmarks coming soon). Run 200/150/125 with identical workloads per `docs/reports/2026-11-08-r5-hardware-validation-guide.md`; restore `berry gc-step 200` after each experiment. Do not change production default without evidence.

## Overall R5 status (partial)

- **Implemented and host-validated:** lifecycle checkpoint instrumentation, extended profile/memory API, GC step-rate command/API, source-buffer ownership verification, host `.be` compile validation (22/22), churn probe script.
- **Hardware evidence collected (partial):** churn probe stable; Breakout cycles 2–4 stable after warm-up (no progressive Berry-managed leak in sample); post-restart second Breakout run diagnosed as **source-buffer allocation failure** (needs 8224 bytes, largest default block 8192) — fragmentation/contiguous-block issue, exposed by `[R5]` logs.
- **Still pending:** full `[R5]` checkpoint 1–7 capture across clean open/close, 10–25 cycle confidence + second app, GC step-rate benchmark matrix (200/150/125), and a measured mitigation/follow-up for post-close default-heap fragmentation causing large app loads to fail `ESP_ERR_NO_MEM`.
