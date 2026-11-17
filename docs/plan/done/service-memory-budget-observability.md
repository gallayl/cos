# Service memory budget observability

- **ID:** service-memory-budget-observability
- **Status:** done (delivered; hardware/manual validation pending human action)
- **Created:** 2026-11-16
- **Updated:** 2026-11-17
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `components/berry_lang/src/berry_runtime.cpp:99-142`, `components/berry_lang/src/berry_runtime.cpp:1629-1676`, `components/network/include/network.h:54-58`, `components/network/include/network.h:192-211`, and general lack of cross-service memory budget in boot/lifecycle. Canonical tracker: [`consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (2026-11-16 audit follow-ups, R54 context). Related: [`./unified-service-lifecycle.md`](./unified-service-lifecycle.md), [`./berry-runtime-service-inspection-v1.md`](./berry-runtime-service-inspection-v1.md) (complementary quick inspection plan; split & linked), R2/R5/R8 evidence in consolidated plan and `docs/reports/`.
- **Approval:** Approved by user in clarify-feature conversation on 2026-11-16 ("Approve as-is and mark Ready (move to docs/plan/ready/)"). Pre-approved for implementation/PR flow (merge remains human).
- **Open questions:** none

## Summary
The firmware has useful local diagnostics (Berry heap/GC checkpoints, bounded network buffers, HTTP worker stack info APIs), but no system-wide view of per-service memory cost and budgets. This feature defines v1 observability needed to manage optional services and understand per-service RAM impact.

**Relationship to `berry-runtime-service-inspection-v1` (split & linked, 2026-11-16):** this plan owns the dedicated `svc_mem` cross-service memory/budget snapshot and before/after evidence workflow. [`berry-runtime-service-inspection-v1.md`](./berry-runtime-service-inspection-v1.md) owns quick operational inspection (`services`, `berry app`, existing `berry profile`/`mem` snapshots). Preserve distinct command roles and output contracts.

### Shared collector contract (resolved, 2026-11-16 review)
- **Owner/order (decided):** `service-memory-budget-observability` **owns** the shared diagnostics contract as a **contract-first prerequisite**. Before `berry-runtime-service-inspection-v1` (`services`/`berry app`) can land, and before consumers such as `optional-network-diagnostics-worker` rely on it, this plan must deliver (in its first milestone/PR, or the same commit series that lands dependent inspection) a concrete shared artifact providing read-only collectors/helpers and the canonical vocabulary. `svc_mem`, `services`, and `berry app` consume this shared contract and must not duplicate gathering logic or define divergent vocabularies.
- **Planned shared artifact (named):** `components/service_diagnostics/include/service_diagnostics.h` with implementation in `components/service_diagnostics/` (component name/path may be finalized in implementation, but docs and consumers must reference this planned artifact consistently until landed). This artifact is owned/delivered by **Milestone 1** of this plan and is the single source of truth for the shared diagnostics contract.
- **Contract-first deliverable (minimum):** shared artifact defines canonical **service IDs/names**, **state strings**, **metric names/units**, and **`n/a`/unknown/unsupported semantics**. Dependent plans code against it; `svc_mem` is built on top of it.
- **Canonical states (minimum vocabulary, extend only in this shared contract):** `uninitialized`/`idle`, `running`/`ready` (or service-specific active form as defined, e.g. `connected`/`clients=<n>` where specified), `stopped`, `failed`, `disabled-by-profile`/`unsupported-in-profile`, `unsupported-stop`/`unsupported` (capability), and `unknown`/`n/a` for unavailable metrics/states. Use identical IDs/names/strings and semantics across `services`, `berry app`, and `svc_mem`; coordinate with `unified-service-lifecycle` states/capabilities once available.
- **Minimum required fields per in-scope service (v1):** every report must include service ID/name and canonical `state` (or explicit `unknown`/`n/a`). In addition: **System heap** — free, min watermark, largest free block (default and internal where distinguishable, else `n/a`); **Berry runtime** — initialized/VM-active state and GC managed bytes (or `n/a`); **Network/HTTP worker** — init/idle/busy state and configured stack size/HWM when available (else `n/a`); **Wi-Fi**, **Bluetooth**, **Text console**, **Shell input** — canonical lifecycle/state and any cheaply exposed stack/buffer/HWM/capacity fields listed in scope (else `n/a` with reason/capability). Additional fields in scope remain optional/"if cheaply exposed" but must use canonical names/units when reported. Any new state/field is defined once in the shared contract and documented in dependent plans. Consumers must not propose or adjust divergent labels locally.

## Scope (v1)
- Provide a **cross-service, on-demand memory snapshot** for developers via a dedicated shell command, reusing/extending existing diagnostics where possible (R5 Berry/runtime, R8 network/HTTP worker stack info, existing `mem`/system info).
- **Command:** `svc_mem` (on-demand, read-only). Prints a grouped text report to the console with a **System/heap** section and one section/line per in-scope service. Existing `mem` and `berry profile` remain stable; implementation may reuse their collectors/APIs.
- **Output contract:** for each service report `state`/enabled (running/stopped/uninitialized/unsupported/unknown as applicable) plus the metrics below; unavailable metrics shown as `n/a` with capability/source noted. Units/labels must be clear and stable enough for manual comparison (bytes; stack HWM in words/bytes as applicable).
- **v1 in-scope services and metric fields** (report if available, else `n/a`):

| Service | State fields | Memory/runtime metrics |
|---|---|---|
| **System heap** | n/a | default heap free, min free (watermark), largest free block; internal heap free/min/largest if distinguishable (align with `mem`/R5 semantics) |
| **Berry runtime** | initialized / VM active / no VM | GC managed bytes (`be_gc_memcount` per R5), GC step rate if exposed, related heap snapshot fields already provided by runtime/profile info |
| **Network / HTTP worker** | network/HTTP worker initialized/idle/busy as available | HTTP worker configured stack size and last/peak HWM if available (R8); bounded body/buffer limits or configured constants where useful for interpretation |
| **Wi-Fi** | init/started/connected/disconnected/stopped/unsupported as available | lifecycle/state; no per-subsystem heap counter assumed (note ESP-IDF reclaimability limits). Include cheaply available counters/buffer sizes only if already exposed |
| **Bluetooth** | init/enabled/disabled/scan state as available | lifecycle/state; scan/result/buffer/task info only if cheaply exposed (no deep instrumentation required in v1) |
| **Text console** | init/deinit, render/task active as available | configured/static scrollback/buffer sizes and task stack size/HWM if cheaply exposed; note ownership/limits (semantics/state definitions follow [`./text-console-memory-lifecycle.md`](./text-console-memory-lifecycle.md), source of truth; hidden/app-owned: UART/stdout forwarding and output listeners preserved, buffer/history preserved while render/touch suspended) |
| **Shell input** | init/task active as available | configured/static input buffer/queue sizes and task stack size/HWM if cheaply exposed |

- **Lifecycle before/after:** mandate `svc_mem` before/after captures for at least one runtime-controllable service start/stop in the implementation context (e.g. via unified service lifecycle or existing service APIs). Require a documented manual R2-style A/B procedure/checklist for in-scope services lacking runtime stop control.
- **Budgets/watermarks:** reporting-only. Specify concrete metric fields in implementation; **reference budgets/watermarks are recorded from R2/R5/R8 and device measurements** and documented in a report/doc (no invented numeric hard targets in spec/code).
- **Overhead:** negligible at idle, on-demand reporting only (no periodic/background sampling).

## Non-goals
- Automatic memory reclamation heuristics without explicit design.
- Full telemetry/remote analytics platform.
- Changing allocator behavior.
- HTTP API or remote reporting surface in v1.
- Runtime budget enforcement, warnings, or blocking actions in v1.
- Continuous/periodic sampling in v1.
- Guaranteeing full ESP-IDF subsystem memory reclamation.

## Acceptance criteria
- [x] `svc_mem` is implemented as an on-demand, read-only shell report with grouped System/heap + in-scope service sections and the specified state/metric fields (or documented `n/a` where unavailable). `mem` and `berry profile` behavior remain stable.
- [x] v1 covers System heap, Berry runtime, Network/HTTP worker, Wi-Fi, Bluetooth, Text console, and Shell input as specified, reusing/extending R2/R5/R8/existing collectors where possible.
- [x] Before/after impact: evidence includes documented `svc_mem`/A-B procedure and checklist for at least one runtime-controllable service start/stop cycle plus R2-style A-B procedure for services lacking runtime stop control (see report). **On-device captures/sign-off remain pending when hardware/environment is unavailable** — concrete blockers/unverified steps recorded per `AGENTS.md` (not claimed as pass).
- [x] Instrumentation is on-demand only with negligible idle overhead; no periodic sampling added. Limitations/overhead are documented.
- [x] Documentation explains metric meanings/units, `n/a` cases, ESP-IDF/non-reclaimable caveats, and how to record/use reference budgets/watermarks from R2/R5/R8 + device measurements (report/doc), with links to existing evidence.
- [x] Automated checks required by `AGENTS.md` for affected scope pass with evidence: M1/M2 implementation landed with recorded automated checks in item history/mainline delivery; this M3 docs-only change reports checks executed in this run (build/lint/hardware manual validation not claimed if unavailable, blockers recorded). **Hardware/manual validation remains required when available** for sign-off of reported values and recorded ranges — recorded as pending human action with concrete blockers.

## Validation plan
- **Automated (required):** run and report per `AGENTS.md`: `make format-check`, `make test`, and `make build`/`make lint`/`make docs-check` as applicable when environments/tools are available (firmware/shell/components/docs touched). If unavailable, report concrete blockers and rerun steps.
- **Hardware/manual (required when hardware/environment available for sign-off of reported values):** on device, capture `svc_mem` (and relevant `mem`/`berry profile` as useful) at baseline and for at least one controllable start/stop cycle; verify reported states/HWMs/heap fields are plausible and stable across repeated reads. Perform/document R2-style A/B observations for services without runtime stop. Record reference ranges/watermarks in docs/report evidence. If unavailable, report concrete blockers and unverified items per `AGENTS.md` (do not claim pass).
- **Out of scope for this item:** exhaustive per-profile/product memory certification beyond captured evidence; deep reclamation fixes owned by related lifecycle items.

## Expected areas/files
- `components/shell/` (register/report command `svc_mem`)
- `components/system/` and/or shared reporting helper for system heap snapshot
- Metric/state accessors or lightweight hooks in: `components/berry_lang/`, `components/network/` (+ HTTP worker), `components/wifi/`, `components/bluetooth/`, `components/text_console/`, shell input implementation files
- `components/service_lifecycle/` alignment/read-only state reuse if useful (no forced coupling)
- `docs/reports/` or docs page for interpretation + reference budgets/watermarks checklist/evidence
- `docs/plan/consolidated-improvement-plan.md` cross-link/status/evidence update when implemented

## Dependencies / prerequisites
- [`./unified-service-lifecycle.md`](./unified-service-lifecycle.md) provides shared lifecycle/state concepts useful for enabled/running/unsupported reporting and for controllable before/after demos (where implemented); consume its states/capabilities as landed.
- [`./berry-runtime-service-inspection-v1.md`](./berry-runtime-service-inspection-v1.md) is complementary (split & linked) and **consumes** this plan's shared diagnostics contract/collectors/vocabulary (contract-first prerequisite, owned/delivered by this plan's Milestone 1 shared artifact `components/service_diagnostics/include/service_diagnostics.h` + `components/service_diagnostics/`; no duplication).
- **Semantic prerequisite:** [`./text-console-memory-lifecycle.md`](./text-console-memory-lifecycle.md) is the source of truth for console lifecycle/behavior semantics and observable states/fields reported for text console; finalize/report console metrics against those semantics.
- Related items R45–R52 may extend stop/deinit/reclaimability; v1 observability must remain correct with `n/a`/unsupported where controls/metrics are unavailable.
- Reuse/align with R2/R5/R8 evidence and workflows in `docs/plan/consolidated-improvement-plan.md` and `docs/reports/`. This plan's `svc_mem`/A-B checklist and canonical state/metric vocabulary are the preferred evidence/reporting template for lifecycle/memory work in ready items.

## Milestones / blockers (explicit)
- [x] **M1 — Shared diagnostics contract artifact (blocker for consumers):** delivered `components/service_diagnostics/include/service_diagnostics.h` + `components/service_diagnostics/` with canonical service IDs/names, state strings, metric names/units, `n/a`/unknown/unsupported semantics, and read-only collectors/helpers. `berry-runtime-service-inspection-v1` (`services`/`berry app`) and other consumers must not land before this (or same commit series) and must not invent divergent vocabularies.
- [x] **M2 — `svc_mem` command/report:** implemented on-demand grouped snapshot using the shared contract/collectors.
- [x] **M3 — Evidence/docs:** before/after captures procedure, A-B checklist alignment, and reference ranges/watermarks recording procedure documented in [`docs/reports/2026-11-17-svc-mem-memory-budget-observability.md`](../../reports/2026-11-17-svc-mem-memory-budget-observability.md). **Device-measured captures/ranges remain pending** (hardware/environment blocker recorded); no fabricated measurements.

## Risks / constraints
- Instrumentation can itself consume RAM/CPU or affect timing; collectors must be read-only and cheap.
- Stack HWMs/heap watermarks are point-in-time and fragmentation-sensitive; values need careful interpretation.
- Some ESP-IDF resources are not reclaimable; reporting must not imply reclamation guarantees.
- Command/output becomes a developer-facing contract; labels/units must be clear and changes documented.
- Per-service internal fields may require small accessor APIs without breaking existing behavior.

## Decisions (recorded)
- **2026-11-16 (clarify-feature, user):**
  - **Audience/surface (v1):** Developers via shell command(s) only; no HTTP API. *(Recommended)*
  - **Coverage (v1):** Core always-on + major optional services — System heap, Berry runtime, Network/HTTP worker, Wi-Fi, Bluetooth, Text console, Shell input. *(Recommended)*
  - **Budgets/enforcement (v1):** Reporting-only with documented reference budgets/watermarks; no runtime enforcement/warnings. *(Recommended)*
  - **R2/R5/R8 & overhead:** Reuse/extend R2/R5/R8 metrics/checklists; on-demand only, negligible idle overhead. *(Recommended)*
- **2026-11-16 (clarify-feature, user, round 2):**
  - **Command contract:** New dedicated shell command `svc_mem` (grouped per-service text/table snapshot); `mem`/`berry profile` remain stable. *(Recommended)*
  - **Lifecycle before/after:** `svc_mem` before/after required for at least one runtime-controllable service; documented R2-style A/B procedure for others. *(Recommended)*
  - **Metrics/budgets specificity:** Concrete metric fields specified in spec; reference budgets/watermarks recorded from measurements in docs/report (no invented hard targets). *(Recommended)*
- **2026-11-16 (clarify-feature, user, approval):** Approved spec as-is and requested move to `docs/plan/ready/` as ready/pre-approved for implementation/PR flow.

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus).
- 2026-11-16 (clarify-feature): reviewed item, `docs/README.md`, consolidated plan (R2/R5/R8, R44–R54), and referenced diagnostics in `berry_runtime.cpp`/`network.h`. Recorded round-1 decisions.
- 2026-11-16 (clarify-feature, round 2): recorded command/before-after/budgets decisions; drafted full spec (scope, metric fields, acceptance criteria, validation plan, areas/files, deps/risks). User confirmed details.
- 2026-11-16 (clarify-feature, approval): explicit approval recorded; status set to ready, open questions none, moved to `docs/plan/ready/service-memory-budget-observability.md`. No implementation or validation run in this clarification step.
- 2026-11-16 (review follow-up): clarified hardware/manual evidence as required when available with explicit blocker reporting for value sign-off/captures (AGENTS-consistent).
- 2026-11-16 (review follow-up 2): established `service-memory-budget-observability` as owner/first-milestone deliverer of the shared diagnostics contract (artifact path, canonical states/IDs/metrics/units/`n/a` semantics, minimum required fields) consumed by `services`/`berry app`/`svc_mem`; no duplication/divergent vocabulary. No implementation/validation yet.
- 2026-11-16 (review follow-up 3): named planned shared artifact `components/service_diagnostics/include/service_diagnostics.h` + `components/service_diagnostics/` as Milestone 1 blocker for consumers and required consumers to follow canonical labels (no local adjustments). No implementation/validation yet.
- 2026-11-17 (implementation, branch `feat/service-memory-budget-observability`, from `origin/main` 0a6190b, green baseline): delivered **M1** shared diagnostics contract artifact (`components/service_diagnostics/include/service_diagnostics.h`, `components/service_diagnostics/src/service_diagnostics.c`, `components/service_diagnostics/CMakeLists.txt`) and **M2** `svc_mem` on-demand shell command/report (`components/shell/src/shell_info_cmds.c`, `components/shell/CMakeLists.txt`) using the shared contract (read-only, no periodic sampling). Added host contract/unit test `test/test_service_diagnostics.c` (+ wiring). Independent read-only review addressed (no `shell`↔`service_diagnostics` CMake dependency cycle; shell input reported as best-effort `ready` without shell coupling). Reported automated checks: `make format` pass, `make format-check` pass, `make test` pass (CTest 27/27, Python `scripts` unittest 3/3 OK), `make docs-check` pass. `make build`/`make lint` not run in that session (ESP-IDF v6.1 / clang-tidy environment not verified); on-device/manual validation not run (no hardware access).
- 2026-11-17 (M3 docs/evidence finalization, branch `feat/svc-mem-m3-evidence` from `origin/main` 472cdc4): added [`docs/reports/2026-11-17-svc-mem-memory-budget-observability.md`](../../reports/2026-11-17-svc-mem-memory-budget-observability.md) documenting `svc_mem` contract/fields/units/`n/a` semantics, required before/after and R2-style A-B checklist/procedure, reference ranges/watermarks recording procedure (aligned with R2/R5/R8), and current evidence/blockers. Marked M1–M3 complete for scoped documentation/evidence deliverables (no fabricated device measurements). Moved item to `docs/plan/done/` and updated consolidated plan cross-links/status in this change. **Remaining human actions:** review/approval and merge (human), and required on-device/manual `svc_mem` captures/sign-off + device-measured reference ranges/watermarks when hardware/environment is available (record evidence in report/plan when completed).
