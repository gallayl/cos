# R20 — 2026-10 deferred backlog (pipes/redirection, editor seam, larger apps, image support)

- **ID:** r20-deferred-backlog
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20, P3, in progress - parsing/design scoped), [`history/2026-10-improvements.md`](../../history/2026-10-improvements.md), reports: [`reports/2026-11-10-r20-pipes-parsing-design.md`](../../reports/2026-11-10-r20-pipes-runtime-design.md), [`reports/2026-11-10-r20-pipes-runtime-design.md`](../../reports/2026-11-10-r20-pipes-runtime-design.md), [`reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md`](../../reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md), [`reports/2026-11-10-r20-redirection-runtime-hw-guide.md`](../../reports/2026-11-10-r20-redirection-runtime-hw-guide.md)
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
R20 bundles multiple deferred items. `>`/`>>` runtime is HW-validated; stderr/FD best-effort runtime implemented (HW smoke pending); pipes have parse/diagnostic foundation and runtime design/proposal but need contract/scope confirmation and implementation; other backlog items (editor seam, larger apps, image preview/support) are not started.

## Scope
- To be split/clarified per sub-item. Candidate sub-items: (a) pipes runtime implementation (scoped contract), (b) stderr/FD HW smoke/remaining redirection semantics, (c) shell line-editor refactor seam, (d) larger apps (separately scoped), (e) image preview/Berry image support feasibility.

## Non-goals
- Unscoped "do all of R20" in one change.
- ESP log-capture-aware redirection or advanced shell semantics unless explicitly scoped/approved.

## Acceptance criteria
- [ ] Sub-items prioritized and each given explicit scope/non-goals, acceptance criteria, validation plan, and approval (separate ready items as appropriate).
- [ ] Pipes runtime (if pursued): contract/scope confirmed, implemented per design constraints, tested (host/integration/HW as applicable), and documented with evidence.
- [ ] Other selected sub-items meet their clarified acceptance criteria and evidence recorded.

## Validation plan
- Automated: `make format-check`, `make test` and scope-appropriate checks per `AGENTS.md` (firmware build/lint, integration/QEMU, frontend/docs-check as applicable).
- Hardware/manual: likely required for shell redirection/pipes behavior and any UI/image work — per sub-item.

## Expected areas/files
- `components/shell/` (parsing/runtime/redirection/editor), `test/`, `integration_test/`, `frontend/` and `components/ui/` or image-related components for later sub-items, docs/reports.

## Dependencies / prerequisites
- Pipes v1 contract/scope confirmation (stdin/consumer contract, supported forms, buffering/limits, diagnostics) per `docs/reports/2026-11-10-r20-pipes-runtime-design.md`.
- Feasibility decisions for larger apps and image support (formats/memory/decoder).

## Risks / constraints
- Memory/stack constraints on ESP32; shell semantics complexity; image decoding feasibility/ROI uncertain.

## Open questions
- [ ] Which R20 sub-item(s) should be prioritized first, and should they be split into separate queue items?
- [ ] **Pipes:** approve/confirm v1 contract and scope (sequential buffered 2-stage pipeline, bounded capture, Berry-first stdin contract, unsupported-form diagnostics, interaction with redirection) sufficient to implement now?
- [ ] **Stderr/FD:** is current best-effort runtime acceptable; what HW smoke/evidence is required to consider it done, and what (if any) log-capture/advanced semantics are in scope?
- [ ] **Editor seam / larger apps / image support:** what concrete scope, acceptance criteria, and feasibility evidence are required before each becomes ready?

## Progress / evidence
- 2026-11-15: created from R20 in-progress/backlog; multiple sub-items require clarification/splitting.