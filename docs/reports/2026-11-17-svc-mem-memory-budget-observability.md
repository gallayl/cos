# Service memory budget observability (`svc_mem`) — M3 evidence, A-B checklist, and reference-budget procedure

- **Date:** 2026-11-17
- **Item:** [`docs/plan/done/service-memory-budget-observability.md`](../plan/done/service-memory-budget-observability.md) (finalized in this change from `plan/ready/`)
- **Scope:** M3 evidence/docs deliverable — `svc_mem` A-B/before-after capture checklist, procedure for services lacking runtime stop control, metric/unit/`n/a` interpretation, and reference budgets/watermarks recording aligned with R2/R5/R8 evidence.
- **Implementation status:** M1 shared diagnostics contract (`components/service_diagnostics/`) and M2 `svc_mem` shell command/report are landed in `main` (read-only, on-demand; no periodic sampling). This report records the M3 procedure and current evidence/blockers.

## 1) Command and contract

- **Command:** `svc_mem` (shell, on-demand, read-only). Registered in `components/shell/src/shell_info_cmds.c`; prints via `service_diag_print_svc_mem()` (`components/service_diagnostics/src/service_diagnostics.c`).
- **Shared contract owner/artifact:** `components/service_diagnostics/include/service_diagnostics.h` and `components/service_diagnostics/src/service_diagnostics.c` (canonical service IDs/names, state strings, metric statuses/units/labels, and `n/a`/`unknown`/`unsupported` semantics). Consumers (`svc_mem`, `services`, `berry app`, diagnostics consumers) must use this contract and must not define divergent vocabularies/collectors.
- **Stability:** existing `mem` and `berry profile` commands/behavior remain unchanged; `svc_mem` reuses/extends existing collectors where available.
- **Overhead:** on-demand snapshot only (no periodic/background sampling). Collection is read-only and intended to be cheap at idle except when explicitly invoked.

## 2) Output groups, metric meanings, units, and semantics

`svc_mem` prints a grouped report. **Units are bytes unless noted.** Each metric is reported as a concrete value (`SERVICE_DIAG_METRIC_OK`) or explicitly as `n/a` / `unknown` / `unsupported` with an optional reason.

| Group / service (canonical ID, name) | State fields | Metrics reported (when available) | Current implementation notes |
| --- | --- | --- | --- |
| `system_heap` — System heap | n/a (heap section) | `default_free`, `default_min_free` (watermark), `default_largest_block`, `internal_free`, `internal_min_free`, `internal_largest_block` (bytes) | Collected from `system_get_info()`; aligns with `mem`/R5 heap semantics where distinguishable. |
| `berry_runtime` — Berry runtime (`state`) | `uninitialized` / `running` / `unknown` (as reported) | `initialized` (bool), `gc_managed` (bytes), `gc_step_rate`, `heap_free`, `heap_min_free`, `heap_largest_block`, `internal_free`, `internal_min_free`, `internal_largest_block` (bytes) | From `berry_runtime_get_memory_info()` (R5-related). `vm_active` tracked internally; report shows initialization/state and managed/heap fields. |
| `network_http` — Network/HTTP worker (`state`) | `idle` / `ready` / `unknown` (as reported) | `worker_stack_cfg` (bytes), `worker_stack_hwm` (bytes), `worker_stack_unit` (bytes/unit), `worker_hwm_available` (bool), note with bounded limits (e.g. `NETWORK_HTTP_BODY_MAX`) | From `network_http_get_worker_stack_info()` (R8). HWM is point-in-time/last-sampled; `no sample yet` reported as `n/a` when unavailable. |
| `wifi` — Wi-Fi (`state`) | canonical state (e.g. `connected` / `running` / `stopped` / `disabled-by-profile` as exposed) | no cheap per-subsystem heap counters exposed | Reports lifecycle/state only; ESP-IDF Wi-Fi reclamation is limited and must not be assumed. |
| `bluetooth` — Bluetooth (`state`) | canonical state (`connected` / `running` / `unknown` / `unsupported-in-profile` depending on build/state) | no cheap scan/buffer/task counters exposed | Best-effort state; enabled-vs-disabled-by-profile distinction may be `unknown` when not exposed by a dedicated API (intentional to avoid false state). |
| `text_console` — Text console (`state`) | canonical state (reported conservatively; may be `unknown` if not cheaply observable) | `cols`, `rows`, `font_size` if exposed, else `unknown`/`n/a` | Task stack/HWM and buffer capacities are not exposed as cheap accessors. **Semantics source of truth:** [`docs/plan/done/text-console-memory-lifecycle.md`](../plan/done/text-console-memory-lifecycle.md) (hidden/app-owned behavior, ownership/limits). |
| `shell_input` — Shell input (`state`) | canonical state (best-effort baseline, e.g. `ready`) | stack/HWM and input buffer/queue sizes not exposed as cheap accessors | Reported conservatively without coupling diagnostics to shell internals; always-on recovery expectations follow shell lifecycle/boot-profile contracts. |

**Additional services in shared snapshot/contract (reported by collectors; `svc_mem` print currently focuses on the v1 in-scope groups above):** `http_server`, `websocket`, `network` (coarse), `time_sync` have canonical IDs/states and fields in `service_diag_snapshot_t` and may be surfaced consistently by consumers (`services`, future `svc_mem` extensions) using the same contract.

### Canonical state vocabulary (minimum; extend only in shared contract)

`uninitialized`/`idle`, `running`/`ready`, `connected` (service-specific active form where applicable), `stopped`, `failed`, `disabled-by-profile`/`unsupported-in-profile`, `unsupported-stop`/`unsupported` (capability), and `unknown`/`n/a` for unavailable metrics/states. Use identical IDs/names/strings/semantics across `svc_mem`, `services`, and `berry app`; coordinate with unified lifecycle states/capabilities.

### `n/a` / `unknown` / `unsupported` and caveats

- **`n/a`:** not applicable for this service/context or not yet sampled (e.g. HTTP worker HWM before any sample) — see reason.
- **`unknown`:** not currently observable cheaply/safely via exposed accessors.
- **`unsupported`:** not supported by platform/service/capability/profile.
- Values are **point-in-time** and heap/fragmentation-sensitive; HWMs/watermarks depend on exercised workload.
- **ESP-IDF/shared-subsystem memory may be non-reclaimable**; reporting must not be interpreted as guaranteed reclamation after stop/disable.
- No runtime budget enforcement/warnings/blocking in v1 (reporting-only).

## 3) A-B / before-after capture checklist (`svc_mem`)

Run on supported hardware when available. Record firmware commit, board/profile/build config, boot conditions (Wi-Fi/BT state, connected peripherals), exact commands/steps, timestamps, and raw `svc_mem` output (and `mem`/`berry profile` as useful) for each capture.

### 3.1 Baseline (all runs)

1. Boot device into a known state (record profile, Wi-Fi/BT/service state). Allow startup to settle.
2. Capture: `svc_mem` (and optionally `mem`, `berry profile`). Label as **Baseline**.
3. Repeat 2–3 times to observe stability; note variance (heap/watermarks are expected to fluctuate with activity).

### 3.2 Required controllable start/stop cycle (before/after)

Perform at least one runtime-controllable service start/stop cycle and capture before/after. Recommended (when available in current firmware/profile): **Wi-Fi managed lifecycle** (or another controllable service with clear start/stop semantics).

1. **Before-stop (service active/running or connected as applicable):** ensure service is in a stable active state; capture `svc_mem` (+ `mem`/`berry profile`). Record service state.
2. **Stop/disable action:** invoke the service control command/API per current implementation/profile constraints (record exact command, e.g. Wi-Fi stop/start commands if available). Wait for state transition to settle.
3. **After-stop:** capture `svc_mem` (+ `mem`/`berry profile`). Record state, heap deltas (free/min/largest), and any HWM/managed-byte changes. Note that ESP-IDF memory may not be reclaimed.
4. **Restart/start action (optional but recommended):** start service again; capture `svc_mem` after return-to-active to check for leaks/growth across cycle.
5. Compare captures: identify plausible deltas, `n/a`/`unknown` fields, and anomalies; do not claim reclamation guarantees.

> If no runtime-controllable start/stop is available in the tested configuration/profile, record the concrete blocker and perform the applicable R2-style A-B observations below instead (do not claim the controllable-cycle capture as complete).

### 3.3 R2-style A-B procedure for services lacking runtime stop control

For services without runtime stop control (or when stop is unsupported/unsafe in the tested context), document and perform A/B observations aligned with R2/R5/R8 workflows:

- **A (reference state):** baseline after boot/settle with the service in its default/profile-allowed state.
- **B (exercise state):** exercise representative workload (e.g. Berry script/app open-close for runtime, HTTP/network requests for worker sampling, BT enable/connect/scan where safe, console visibility/app ownership transitions per text-console semantics) and capture `svc_mem`/related diagnostics at defined checkpoints.
- Record before/after (or A/B) deltas, watermarks/HWMs observed, workload details, and limitations (e.g. HWM only after sample, fields not exposed, non-reclaimable resources).
- Reuse existing evidence/checklists where applicable: R2 profiling/checkpoints (`docs/reports/2026-11-08-r2-profiling-checklist.md`), R5 lifecycle/heap checkpoints and GC evidence (`docs/reports/2026-11-08-r5-hardware-results-partial.md`, `docs/reports/2026-11-08-r5-hardware-validation-guide.md`), R8 HTTP worker stack/TLS/abandon measurements (`docs/reports/2026-11-08-r8-http-stack-tls-measurement-guide.md`).

## 4) Reference budgets/watermarks (record from measurements; no invented targets)

v1 is **reporting-only**: do not invent numeric hard budgets in code/spec. Record reference ranges/watermarks from R2/R5/R8 and device measurements in this report (or linked reports) as evidence is captured.

| Area | Reference evidence (existing) | Reference ranges/watermarks (device-measured) | Status |
| --- | --- | --- | --- |
| System/default/internal heap | R2/R5/R8 reports (boot/runtime/workload/watermarks) | **Pending device capture in this environment** (see blockers). Use R2/R5/R8 linked reports for prior measured values and contexts. | Pending |
| Berry runtime GC/heap | R5 lifecycle/checkpoints, `berry profile`/runtime info, R2 | **Pending device capture in this environment**; see R5 report for prior validated measurements. | Pending |
| Network/HTTP worker stack HWM/config | R8 measurements (stack kept 6144; observed HWM/margins), `NETWORK_HTTP_BODY_MAX` limits | **Pending device capture in this environment**; see R8 report (e.g. worst observed HWM 2184/6144 in captured cases). | Pending |
| Wi-Fi / BT / Text console / Shell input | Lifecycle plans + A/B observations | **Pending device capture in this environment** (state/capacity fields mostly `n/a`/`unknown` by design). | Pending |

When hardware measurements are captured, append: date/commit/profile, capture table (baseline/before/after), observed ranges/watermarks, and any anomalies/follow-ups (link related items/issues if filed).

## 5) Evidence status (2026-11-17)

### Automated / code (landed in `main`)

- M1 shared diagnostics contract artifact: `components/service_diagnostics/include/service_diagnostics.h`, `components/service_diagnostics/src/service_diagnostics.c`, `components/service_diagnostics/CMakeLists.txt`.
- M2 `svc_mem` command: `components/shell/src/shell_info_cmds.c` (`cmd_svc_mem`, `esp_console_cmd_register`), `components/shell/CMakeLists.txt` wiring; grouped on-demand report via `service_diag_print_svc_mem()`.
- Host test: `test/test_service_diagnostics.c` (+ wiring) for canonical vocabulary/helpers and snapshot/collect behavior under host mocks.
- Prior recorded automated checks (from item progress, pre-fix/post-fix context on implementation branch/history): `make format` pass, `make format-check` pass, `make test` pass (CTest incl. `test_service_diagnostics`, Python scripts unittest), `make docs-check` pass. **This M3 docs-only change:** see PR/commit validation evidence for checks executed in this run (reported per `AGENTS.md`; do not claim unrun checks).

### Hardware/manual validation (required when available; not executed in this environment)

- **Blocker:** no target ESP32/CYD hardware/device access in this environment; on-device `svc_mem` captures cannot be produced here.
- **Unverified/pending (human action):** baseline and controllable start/stop before/after `svc_mem` captures on device, sign-off that reported states/HWMs/heap fields are plausible/stable across repeated reads, R2-style A/B observations for services without runtime stop, and device-measured reference ranges/watermarks recorded in this report. Until captured, treat hardware-dependent acceptance as pending with concrete blocker recorded (not a pass claim).
- **How to complete:** follow sections 3.1–3.3 on device, then append captured outputs/measurements to section 4 and update the plan item/consolidated tracker evidence links.

## 6) Related evidence and links

- Tracker: [`docs/plan/consolidated-improvement-plan.md`](../plan/consolidated-improvement-plan.md) (R2/R5/R8 and 2026-11-16 audit follow-ups; `svc_mem` as preferred evidence template).
- R2: [`docs/reports/2026-11-08-r2-profiling-checklist.md`](./2026-11-08-r2-profiling-checklist.md)
- R5: [`docs/reports/2026-11-08-r5-hardware-validation-guide.md`](./2026-11-08-r5-hardware-validation-guide.md), [`docs/reports/2026-11-08-r5-hardware-results-partial.md`](./2026-11-08-r5-hardware-results-partial.md)
- R8: [`docs/reports/2026-11-08-r8-http-stack-tls-measurement-guide.md`](./2026-11-08-r8-http-stack-tls-measurement-guide.md), [`docs/reports/2026-11-09-r8-followups-https-heap.md`](./2026-11-09-r8-followups-https-heap.md)
- Console semantics: [`docs/plan/done/text-console-memory-lifecycle.md`](../plan/done/text-console-memory-lifecycle.md)
- Complementary inspection: [`docs/plan/done/berry-runtime-service-inspection-v1.md`](../plan/done/berry-runtime-service-inspection-v1.md)
- Shared lifecycle: [`docs/plan/ready/unified-service-lifecycle.md`](../plan/ready/unified-service-lifecycle.md)
