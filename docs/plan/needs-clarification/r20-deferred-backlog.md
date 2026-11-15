# R20 — 2026-10 deferred backlog (pipes/redirection, editor seam, larger apps, image support)

- **ID:** r20-deferred-backlog
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20, P3, in progress - parsing/design scoped), [`history/2026-10-improvements.md`](../../history/2026-10-improvements.md), reports: [`reports/2026-11-10-r20-pipes-parsing-design.md`](../../reports/2026-11-10-r20-pipes-runtime-design.md), [`reports/2026-11-10-r20-pipes-runtime-design.md`](../../reports/2026-11-10-r20-pipes-runtime-design.md), [`reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md`](../../reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md), [`reports/2026-11-10-r20-redirection-runtime-hw-guide.md`](../../reports/2026-11-10-r20-redirection-runtime-hw-guide.md)
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
R20 bundles multiple deferred items. `>`/`>>` runtime is HW-validated; stderr/FD best-effort runtime implemented (HW smoke pending); pipes have parse/diagnostic foundation and runtime design/proposal but need contract/scope confirmation and implementation; other backlog items (editor seam, larger apps, image preview/support) are not started.

## Scope
- **Umbrella item (remaining after split):** track and later clarify (b) stderr/FD HW smoke/remaining redirection semantics, (c) shell line-editor refactor seam, (d) larger apps (separately scoped), (e) image preview/Berry image support feasibility.
- **Clarified/promoted as separate target:** (a) pipes runtime implementation v1 (scoped contract) — **Ready:** [`../ready/r20-pipes-runtime-v1.md`](../ready/r20-pipes-runtime-v1.md).

## Non-goals
- Unscoped "do all of R20" in one change.
- ESP log-capture-aware redirection or advanced shell semantics unless explicitly scoped/approved.
- Pipes v1 scope/contract decisions beyond what is specified/approved in `r20-pipes-runtime-v1.md`.

## Acceptance criteria
- [ ] Remaining sub-items (b–e) prioritized and each given explicit scope/non-goals, acceptance criteria, validation plan, and approval (separate queue items as appropriate).
- [x] Pipes runtime v1 clarified, explicitly approved, and moved to Ready as a separate item (`../ready/r20-pipes-runtime-v1.md`); umbrella retained for the rest (2026-11-15).
- [ ] Other selected sub-items meet their clarified acceptance criteria and evidence recorded.

## Validation plan
- Automated: `make format-check`, `make test` and scope-appropriate checks per `AGENTS.md` (firmware build/lint, integration/QEMU, frontend/docs-check as applicable).
- Hardware/manual: likely required for shell redirection/pipes behavior and any UI/image work — per sub-item.

## Expected areas/files
- `components/shell/` (parsing/runtime/redirection/editor), `test/`, `integration_test/`, `frontend/` and `components/ui/` or image-related components for later sub-items, docs/reports.

## Dependencies / prerequisites
- Pipes v1 specification/approval recorded in [`../ready/r20-pipes-runtime-v1.md`](../ready/r20-pipes-runtime-v1.md) (Ready, approved 2026-11-15). Implementation/validation evidence still pending.
- Feasibility decisions for larger apps and image support (formats/memory/decoder).

## Risks / constraints
- Memory/stack constraints on ESP32; shell semantics complexity; image decoding feasibility/ROI uncertain.

## Decisions / resolved questions (2026-11-15)
- [x] **Prioritized sub-item for clarification (2026-11-15):** Pipes runtime v1 specified as a separate queue item and made Ready after explicit approval.
- [x] **Split/structure:** keep this umbrella item in `needs-clarification` for remaining R20 sub-items; pipes v1 tracked in `docs/plan/ready/r20-pipes-runtime-v1.md`.

## Open questions (remaining for umbrella R20)
- [ ] **Stderr/FD:** is current best-effort runtime acceptable; what HW smoke/evidence is required to consider it done, and what (if any) log-capture/advanced semantics are in scope?
- [ ] **Editor seam / larger apps / image support:** what concrete scope, acceptance criteria, and feasibility evidence are required before each becomes ready?
- [ ] Prioritization/order for remaining sub-items after pipes v1 clarification/implementation.

## Progress / evidence
- 2026-11-15: created from R20 in-progress/backlog; multiple sub-items require clarification/splitting.
- 2026-11-15 (clarification): prioritized Pipes runtime v1 for Ready clarification (separate item) and kept this umbrella for remaining sub-items (user decisions). `r20-pipes-runtime-v1.md` specified (scope/contract/AC/validation/dependencies/risks, open questions none), **explicitly approved by user**, and moved to `docs/plan/ready/r20-pipes-runtime-v1.md` (pre-approved for implementation/PR flow; merge remains human). Remaining R20 sub-items (stderr/FD completion evidence, editor seam, larger apps, image support) still need clarification/prioritization.