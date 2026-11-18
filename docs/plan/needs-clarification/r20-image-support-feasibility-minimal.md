# R20e: Image support feasibility spike → minimal Berry/UI image display

- **ID:** r20-image-support-feasibility-minimal
- **Status:** needs-clarification
- **Created:** 2026-11-18
- **Source/links:** docs/plan/needs-clarification/r20-deferred-backlog.md (umbrella, item e: image preview/Berry image support feasibility); docs/plan/consolidated-improvement-plan.md (R20); Berry UI components (components/ui/*), frontend/docs history context
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
Run the deferred feasibility for image preview/support on ESP32 (CYD) and, if viable, deliver a minimal slice that allows displaying a constrained image from filesystem in a Berry/UI context (e.g. small RGB565/raw or a tightly bounded decoded format). Goal: move UI from rectangles/text/demo-only toward practical asset display with explicit memory/format limits.

## Scope
- Feasibility assessment: candidate formats/decoders, RAM/flash/stack impact, licensing, performance, and compatibility with existing UI stack (LovyanGFX/display pipeline).
- If feasible, define a minimal v1 slice (format set, max dimensions/size, loading path from VFS, error handling) with acceptance criteria and validation plan.
- Document hard limits and non-goals clearly (no general image pipeline in v1 unless justified).

## Non-goals
- Full codec support matrix or general-purpose image library integration beyond minimal v1.
- Image editing, animation, or complex compositing beyond the scoped minimal display use case.
- UI redesign of existing apps beyond what is needed for a reference example (separately tracked if desired).

## Acceptance criteria
- [ ] Feasibility report section captured in this item (or linked report) with recommendation (go/no-go) and measured/estimated constraints.
- [ ] If go: minimal v1 API/behavior specified (format, size caps, VFS path rules, failure modes) and a reference example is planned.
- [ ] If no-go: documented rationale, blockers, and alternative follow-ups (defer/close) recorded.

## Validation plan
- Automated: make format-check, make test and any host/unit tests added for parsing/logic (if applicable); build/lint/docs-check per scope/AGENTS.md.
- Hardware/manual: required for on-device display/decoder validation when go; measure memory (heap/largest block/watermarks as relevant) and document results; if unavailable record blockers.

## Expected areas/files
- components/ui/*, components/filesystem/*, Berry UI bindings (components/berry_lang/src/berry_ui.cpp and related), frontend/src/cos-sdk/ui.d.ts (if API exposed), storage/flash_data examples, docs/reports.

## Dependencies / prerequisites
- R20 umbrella (docs/plan/needs-clarification/r20-deferred-backlog.md).
- Understanding of current display/LovyanGFX usage and memory budgets (R2/R5/R8 context).

## Risks / constraints
- Tight ESP32 RAM/PSRAM assumptions must be explicit; decoders can be memory-heavy.
- May conflict with performance/stack budgets; require evidence before committing.

## Open questions
- [ ] Which formats are in scope for v1 (e.g. raw RGB565, BMP, PNG subset, JPEG subset) and why?
- [ ] Where decoding runs (firmware component vs Berry helpers) and memory ownership model?
- [ ] What are acceptable max image dimensions/file sizes for CYD?
- [ ] Is on-device filesystem loading from /flash and/or /sdcard both required?

## Progress / evidence
- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 2).
