# R20e: Image support feasibility spike → minimal Berry/UI image display

- **ID:** r20-image-support-feasibility-minimal
- **Status:** needs-clarification
- **Created:** 2026-11-18
- **Source/links:** docs/plan/needs-clarification/r20-deferred-backlog.md (umbrella, item e: image preview/Berry image support feasibility); docs/plan/consolidated-improvement-plan.md (R20); Berry UI components (components/ui/*), frontend/docs history context
- **Approval:** pending (ready/pre-approved asked after feature specification)

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
- [ ] Feasibility report (in-item section or linked `docs/reports/*`) records: candidate approaches considered, estimated/ measured memory (RAM/flash/stack) and performance notes, licensing/complexity notes, compatibility with LovyanGFX/display pipeline, and a clear **go/no-go** recommendation.
- [ ] **If go:** minimal v1 spec is documented (target format confirmation or change with rationale, exact caps/dimensions/file-size rules, VFS path rules for `/flash` and `/sdcard`, error/failure modes, proposed Berry/TS API shape at a high level, reference example plan) and marked ready for a follow-up implementation item/PR if desired.
- [ ] **If no-go:** documented rationale, blockers, and concrete alternative follow-ups (defer/close/other direction) recorded.
- [ ] Automated checks guidance noted for any future code changes (per `AGENTS.md`); this spike item itself may be docs/reports only.

## Validation plan
- **Automated (if any code/tools added):** `make format-check`, `make test`; `make build`/`make lint`/`make docs-check` as scope requires (per `AGENTS.md`).
- **Hardware/manual:** required for minimal-slice validation **if go and implemented later** (CYD): load+draw cases from `/flash` and `/sdcard`, measure heap/largest block/watermarks around operations, document results. For report-only/no-go, hardware is optional except to sanity-check assumptions; record what was not measured.

## Expected areas/files
- `components/ui/*` (render/draw path), `components/filesystem/*` (VFS reads), `components/berry_lang/src/berry_ui.cpp` and related bindings, `frontend/src/cos-sdk/ui.d.ts` (if API added), example assets under `storage/flash_data/` or test paths, `docs/reports/*` for feasibility evidence.

## Dependencies / prerequisites
- R20 umbrella: `docs/plan/needs-clarification/r20-deferred-backlog.md` (item e).
- Current display/LovyanGFX pipeline and memory context (R2/R5/R8 reports) for budgeting.

## Risks / constraints
- ESP32 RAM/PSRAM assumptions must be explicit; on-device decode/line buffers can fragment or exhaust heap.
- Performance/stack limits may require tight caps; caps in 4A are tentative until measured.
- Any decoder choice must respect licensing and flash/RAM budget.

## Open questions
- None (decisions recorded 2026-11-18: 1A, 2A, 3A, 4A with tentative 96×96 / ~20 KiB RGB565 to validate in spike, 5A). If spike reveals hard blockers, record them here and do not claim go.

## Progress / evidence
- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 2).
- 2026-11-18 (clarify-feature): reviewed `components/ui/*`, R20 umbrella context. User choices recorded (feasibility-first + optional minimal v1 spec if go; raw RGB565 target direction; firmware/UI + Berry/TS bindings; load from `/flash` and `/sdcard` with tentative caps 96×96/~20 KiB RGB565 to validate; CYD HW required for minimal-slice validation if go). Awaiting explicit approval to move to `docs/plan/ready/r20-image-support-feasibility-minimal.md`.
