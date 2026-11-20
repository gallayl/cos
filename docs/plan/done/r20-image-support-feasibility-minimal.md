# R20e: Image support feasibility spike → minimal Berry/UI image display

- **ID:** r20-image-support-feasibility-minimal
- **Status:** done (feasibility spike delivered, **go** recommendation recorded; follow-up minimal-slice implementation not started)
- **Created:** 2026-11-18
- **Completed (spike):** 2026-11-20
- **Source/links:** [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (umbrella, item e: image preview/Berry image support feasibility); [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20); Berry UI components (`components/ui/*`), frontend/docs history context
- **Feasibility report:** [`../../reports/2026-11-20-r20e-image-support-feasibility.md`](../../reports/2026-11-20-r20e-image-support-feasibility.md)
- **Approval:** user (2026-11-20, clarify-feature conversation) — explicit approval to mark ready and move to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.

## Summary

Run the deferred feasibility for image preview/support on ESP32 (CYD). This item is a **feasibility spike + written recommendation first** (go/no-go). If **go**, define a **minimal v1 spec** in the same item (or linked doc update) so implementation is a follow-up; if **no-go**, record rationale, blockers, and alternatives. Initial target direction (to validate in spike): constrained raw RGB565 (or preconverted) display from VFS via firmware/UI path with Berry/Cos SDK bindings.

## Scope (decisions recorded 2026-11-18)

- **Outcome model (1A):** feasibility report with go/no-go and measured/estimated constraints captured in this item and/or a linked `docs/reports/*`. If go, include minimal v1 spec (format, caps, API shape, VFS rules, error/failure modes, example plan). If no-go, include rationale and follow-up alternatives (defer/close guidance).
- **Target format direction (2A, validate in spike):** prioritize **raw RGB565 (or preconverted)** from filesystem as the smallest useful path; spike should evaluate feasibility vs BMP/PNG/JPEG subsets only as comparison notes if needed, but v1 target is raw/preconverted unless spike shows a clearly better bounded option (still must meet memory/complexity constraints).
- **Architecture (3A):** **firmware/UI-layer primitive + Berry/Cos SDK binding**. Decoding/drawing and memory ownership controlled in firmware/components (e.g. `components/ui` + VFS helpers); Berry/TS exposed via thin API for constrained calls (exact signature decided in spec if go).
- **VFS sources and tentative caps (4A, validate in spike):** support loading from **`/flash` and `/sdcard`** when available (subject to mount/availability). **Tentative starting caps (hypothesis):** e.g. **≤ 96×96 px** and/or **≤ ~20 KiB** RGB565 (to be confirmed/adjusted by spike measurements). Single image operation per call/context as scoped; no unbounded streaming decode beyond caps.
- **Validation expectation (5A):** if a minimal slice is implemented later from this work, **CYD hardware validation required** (heap/largest block/watermarks around load+draw, aligned with R2/R5 style reporting). Feasibility/report-only may rely on estimates but must state assumptions.

## Non-goals

- Full codec support matrix or general-purpose image library beyond a minimal v1 if approved.
- Image editing, animation, complex compositing, or broad UI redesign beyond a reference example if needed.
- Promising support for PNG/JPEG/BMP in v1 without spike evidence.

## Acceptance criteria

- [x] Feasibility report (in-item section or linked `docs/reports/*`) records: candidate approaches considered, estimated/ measured memory (RAM/flash/stack) and performance notes, licensing/complexity notes, compatibility with LovyanGFX/display pipeline, and a clear **go/no-go** recommendation. → [`docs/reports/2026-11-20-r20e-image-support-feasibility.md`](../../reports/2026-11-20-r20e-image-support-feasibility.md)
- [x] **If go:** minimal v1 spec is documented (target format confirmation or change with rationale, exact caps/dimensions/file-size rules, VFS path rules for `/flash` and `/sdcard`, error/failure modes, proposed Berry/TS API shape at a high level, reference example plan) and marked ready for a follow-up implementation item/PR if desired. → report §8 (conditional go; follow-up implementation item/PR to be created/scheduled separately if pursued).
- [x] **If no-go:** documented rationale, blockers, and concrete alternative follow-ups (defer/close/other direction) recorded. → N/A (recommendation is **go, conditional/minimal v1 only**); non-go directions for PNG/JPEG/general decode documented in report.
- [x] Automated checks guidance noted for any future code changes (per `AGENTS.md`); this spike item itself may be docs/reports only. → report §9.

## Validation plan

- **Automated (if any code/tools added):** `make format-check`, `make test`; `make build`/`make lint`/`make docs-check` as scope requires (per `AGENTS.md`).
- **Hardware/manual:** required for minimal-slice validation **if go and implemented later** (CYD): load+draw cases from `/flash` and `/sdcard`, measure heap/largest block/watermarks around operations, document results. For report-only/no-go, hardware is optional except to sanity-check assumptions; record what was not measured.

## Expected areas/files

- `components/ui/*` (render/draw path), `components/filesystem/*` (VFS reads), `components/berry_lang/src/berry_ui.cpp` and related bindings, `frontend/src/cos-sdk/ui.d.ts` (if API added), example assets under `storage/flash_data/` or test paths, `docs/reports/*` for feasibility evidence.

## Dependencies / prerequisites

- R20 umbrella: [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (item e).
- Current display/LovyanGFX pipeline and memory context (R2/R5/R8 reports) for budgeting.

## Risks / constraints

- ESP32 RAM/PSRAM assumptions must be explicit; on-device decode/line buffers can fragment or exhaust heap.
- Performance/stack limits may require tight caps; caps in 4A are tentative until measured.
- Any decoder choice must respect licensing and flash/RAM budget.

## Open questions

- None (decisions recorded 2026-11-18: 1A, 2A, 3A, 4A with tentative 96×96 / ~20 KiB RGB565 to validate in spike, 5A). If spike reveals hard blockers, record them here and do not claim go. No blocking unknowns found in spike scope; implementation-time decisions (header vs explicit dimensions, error-code mapping, clip-vs-reject bounds, byte-order convention) remain bounded and must be finalized with hardware validation in follow-up work.

## Progress / evidence

- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 2).
- 2026-11-18 (clarify-feature): reviewed `components/ui/*`, R20 umbrella context. User choices recorded (feasibility-first; raw RGB565 target; firmware/UI + Berry/TS bindings; `/flash`+`/sdcard` with tentative caps; CYD HW required for minimal-slice validation if go). Open questions resolved.
- 2026-11-20 (clarify-feature, user approval): explicit approval recorded (user, clarify-feature conversation) to mark **ready (approved)** and move from `docs/plan/needs-clarification/` to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.
- 2026-11-20 (implementation, spike): investigated `components/display/{include/display.h,src/display.cpp,src/cyd_board_config.h,idf_component.yml}`, `components/ui/{include/ui.h,src/ui.cpp}`, `components/berry_lang/src/berry_ui.cpp`, `frontend/src/cos-sdk/ui.d.ts`, `components/filesystem/include/filesystem.h`, and memory context in `docs/reports/2026-11-08-r2-profiling-checklist.md`, `docs/reports/2026-11-08-r5-hardware-results-partial.md`, `docs/reports/2026-11-09-r8-followups-https-heap.md`. Delivered feasibility report with **conditional go** recommendation and minimal v1 spec: [`../../reports/2026-11-20-r20e-image-support-feasibility.md`](../../reports/2026-11-20-r20e-image-support-feasibility.md). **Hardware not available; no on-device measurements performed.** Estimates/assumptions are documented in the report; CYD load+draw + heap/largest-block/watermark validation remains required for any follow-up minimal-slice implementation.
- 2026-11-20 (delivery): PR [#107](https://github.com/gallayl/cos/pull/107) opened from `docs/r20e-image-support-feasibility` (commit `3373e54`, docs follow-up commits update evidence/CI status). Moved item `ready/` → `done/` and updated cross-links in consolidated plan and R20 umbrella. Local validation recorded: `make format-check` pass, `make test` pass (CTest 31/31, Python `scripts` unittest 3/3 OK), `make docs-check` pass. PR CI run [44747032796](https://github.com/gallayl/cos/actions/runs/44747032796) completed **success** (all required checks green; non-applicable jobs skipped as path-filtered for docs-only changes). Independent evidence-based doc/review pass: no actionable findings. Remaining human actions: review/approval and merge. Follow-up implementation (if pursued) requires a separate clarified/ready item or approved scope with CYD hardware validation evidence.