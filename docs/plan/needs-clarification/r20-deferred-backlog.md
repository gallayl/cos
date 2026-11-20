# R20 — 2026-10 deferred backlog (pipes/redirection, editor seam, larger apps, image support)

- **ID:** r20-deferred-backlog
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20, P3, in progress - parsing/design scoped), [`history/2026-10-improvements.md`](../../history/2026-10-improvements.md), reports: [`reports/2026-11-10-r20-pipes-parsing-design.md`](../../reports/2026-11-10-r20-pipes-runtime-design.md), [`reports/2026-11-10-r20-pipes-runtime-design.md`](../../reports/2026-11-10-r20-pipes-runtime-design.md), [`reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md`](../../reports/2026-11-10-r20-runtime-pipes-stderr-design-spike.md), [`reports/2026-11-10-r20-redirection-runtime-hw-guide.md`](../../reports/2026-11-10-r20-redirection-runtime-hw-guide.md)
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
R20 bundles multiple deferred items. `>`/`>>` runtime is HW-validated; stderr/FD best-effort runtime implemented (HW smoke pending); pipes have parse/diagnostic foundation and runtime design/proposal but need contract/scope confirmation and implementation; other backlog items (editor seam, larger apps, image preview/support) are not started.

## Scope
- **Umbrella item (remaining after split):** track remaining R20 coordination/prioritization and any still-open questions not covered by split items.
- **Clarified/promoted as separate items:** (a) pipes runtime implementation v1 — **Done:** [`../done/r20-pipes-runtime-v1.md`](../done/r20-pipes-runtime-v1.md). (b) stderr/FD hardened evidence + logging playbook — **Done (docs/playbook/evidence delivered; CYD HW smoke evidence pending human):** [`../done/r20-stderr-fd-evidence-logging-playbook.md`](../done/r20-stderr-fd-evidence-logging-playbook.md), report [`../../reports/2026-11-20-r20b-stderr-fd-evidence-playbook.md`](../../reports/2026-11-20-r20b-stderr-fd-evidence-playbook.md), PR [#111](https://github.com/gallayl/cos/pull/111). (c) shell line-editor seam + script workflow polish — **Done:** [`../done/r20-shell-line-editor-script-workflow.md`](../done/r20-shell-line-editor-script-workflow.md) (PR [#109](https://github.com/gallayl/cos/pull/109), CI green; hardware/manual validation pending). (d) Berry/UI larger app patterns + reference template — **Done (delivered, PR [#110](https://github.com/gallayl/cos/pull/110), CI [44764432642](https://github.com/gallayl/cos/actions/runs/44764432642) green; required on-device CYD smoke pending human validation):** [`../done/r20-berry-ui-larger-app-patterns.md`](../done/r20-berry-ui-larger-app-patterns.md), guide [`../../berry-ui-larger-app-patterns.md`](../../berry-ui-larger-app-patterns.md). (e) image support feasibility spike / minimal spec — **Done (feasibility spike, conditional go):** [`../done/r20-image-support-feasibility-minimal.md`](../done/r20-image-support-feasibility-minimal.md), report [`../../reports/2026-11-20-r20e-image-support-feasibility.md`](../../reports/2026-11-20-r20e-image-support-feasibility.md). Follow-up minimal-slice implementation (if pursued) requires separate clarified/approved item with CYD hardware validation.

## Non-goals
- Unscoped "do all of R20" in one change.
- ESP log-capture-aware redirection or advanced shell semantics unless explicitly scoped/approved.
- Pipes v1 scope/contract decisions beyond what is specified/approved in `r20-pipes-runtime-v1.md`.

## Acceptance criteria
- [x] Remaining sub-items (b–e) prioritized and each given explicit scope/non-goals, acceptance criteria, validation plan, and approval as separate queue items (2026-11-18 specs; explicitly approved and moved to `ready/` 2026-11-20).
- [x] Pipes runtime v1 clarified, explicitly approved, and delivered (moved to `done/`); umbrella retained for coordination (2026-11-15/2026-11-17).
- [ ] Selected sub-items meet their clarified acceptance criteria and evidence recorded (tracked in their respective ready/done items). Umbrella-level prioritization/order questions remain only if further R20 follow-ups beyond (b–e) are proposed.

## Validation plan
- Automated: `make format-check`, `make test` and scope-appropriate checks per `AGENTS.md` (firmware build/lint, integration/QEMU, frontend/docs-check as applicable).
- Hardware/manual: likely required for shell redirection/pipes behavior and any UI/image work — per sub-item.

## Expected areas/files
- `components/shell/` (parsing/runtime/redirection/editor), `test/`, `integration_test/`, `frontend/` and `components/ui/` or image-related components for later sub-items, docs/reports.

## Dependencies / prerequisites
- Pipes v1 specification/approval recorded in [`../ready/r20-pipes-runtime-v1.md`](../ready/r20-pipes-runtime-v1.md) (Ready, approved 2026-11-15). Implementation/validation evidence still pending.
- Feasibility decisions for larger apps and image support (formats/memory/decoder). Image-support feasibility spike (e) delivered 2026-11-20 with conditional-go recommendation and proposed minimal v1 spec (raw/preconverted RGB565, streamed VFS path) in [`../../reports/2026-11-20-r20e-image-support-feasibility.md`](../../reports/2026-11-20-r20e-image-support-feasibility.md); follow-up implementation remains a separate item pending approval/scheduling and mandatory CYD hardware validation. Larger-apps patterns/template remain tracked in item (d).

## Risks / constraints
- Memory/stack constraints on ESP32; shell semantics complexity; image decoding feasibility/ROI uncertain.

## Decisions / resolved questions (2026-11-15)
- [x] **Prioritized sub-item for clarification (2026-11-15):** Pipes runtime v1 specified as a separate queue item and made Ready after explicit approval.
- [x] **Split/structure:** keep this umbrella item in `needs-clarification` for remaining R20 sub-items; pipes v1 tracked in `docs/plan/ready/r20-pipes-runtime-v1.md`.

## Open questions (remaining for umbrella R20)
- None for the split sub-items (a–e); those are tracked in their own ready/done items with `Open questions: none` (b–e approved/moved to `ready/` 2026-11-20). If new R20 follow-ups beyond (a–e) are proposed (e.g. ESP log-capture-aware redirection, advanced shell semantics, additional editor/app/image work), create/specify them as separate items with explicit scope, acceptance criteria, validation, dependencies/risks, and approval before implementation. This umbrella remains in `needs-clarification` as a coordination record only while no such new follow-up is specified/approved. (No unanswered user questions blocking the umbrella as currently scoped.)

## Progress / evidence
- 2026-11-15: created from R20 in-progress/backlog; multiple sub-items require clarification/splitting.
- 2026-11-15 (clarification): prioritized Pipes runtime v1 for Ready clarification (separate item) and kept this umbrella for remaining sub-items (user decisions). `r20-pipes-runtime-v1.md` specified, **explicitly approved by user**, and moved to `docs/plan/ready/r20-pipes-runtime-v1.md` (pre-approved for implementation/PR flow; merge remains human).
- 2026-11-18: separate needs-clarification items created for sub-items b–e with recorded scope decisions and `Open questions: none` pending approval (`r20-stderr-fd-evidence-logging-playbook`, `r20-shell-line-editor-script-workflow`, `r20-berry-ui-larger-app-patterns`, `r20-image-support-feasibility-minimal`).
- 2026-11-20 (clarify-feature, user approval): explicit approval recorded to move sub-items b–e to `docs/plan/ready/` (pre-approved for implementation/PR flow per global gates; merge remains human). Umbrella updated to reflect split/ready items and no unanswered user questions for currently scoped umbrella coordination. Any new R20 follow-ups beyond (a–e) require separate clarification/spec/approval.
- 2026-11-20 (implementation, spike delivery): sub-item (e) `r20-image-support-feasibility-minimal` delivered as docs/report-only feasibility spike (conditional **go**, raw/preconverted RGB565 streamed VFS path with minimal v1 spec) and moved to `docs/plan/done/`; report [`../../reports/2026-11-20-r20e-image-support-feasibility.md`](../../reports/2026-11-20-r20e-image-support-feasibility.md). No hardware validation performed; follow-up minimal-slice implementation requires separate approved item and CYD hardware validation.
- 2026-11-20 (implementation/delivery): sub-item (c) `r20-shell-line-editor-script-workflow` delivered (key/escape parser seam, `which` resolver command, improved script resolver/not-found messaging) and moved to `docs/plan/done/`. PR [#109](https://github.com/gallayl/cos/pull/109), CI green; hardware/manual validation pending (human).
- 2026-11-20 (implementation, template delivery): sub-item (d) `r20-berry-ui-larger-app-patterns` delivered as annotated onboarding template app (`storage/ts/apps/onboarding.ts`, compiled `storage/flash_data/apps/onboarding.be`) plus guide `docs/berry-ui-larger-app-patterns.md`; moved to `docs/plan/done/`. Automated local checks recorded in item. PR [#110](https://github.com/gallayl/cos/pull/110), CI run [44764432642](https://github.com/gallayl/cos/actions/runs/44764432642) green/all required checks passed; required on-device CYD smoke/heap notes pending human validation (merge-blocking comment posted, evidence/waiver required).
- 2026-11-20 (implementation/delivery, docs-only): sub-item (b) `r20-stderr-fd-evidence-logging-playbook` delivered (canonical HW smoke matrix/playbook + end-to-end shell session example with `2>`/`2>>`/`&>`/append-truncate sanity and `ESP_LOG*`/`&>` callouts, README user-facing note, design-doc cross-link). On-device CYD HW smoke not executed (no device/serial console, SD availability unknown); blockers and human remediation/collection steps recorded. PR [#111](https://github.com/gallayl/cos/pull/111), CI [44767477786](https://github.com/gallayl/cos/actions/runs/44767477786) green; item moved to [`../done/r20-stderr-fd-evidence-logging-playbook.md`](../done/r20-stderr-fd-evidence-logging-playbook.md), report [`../../reports/2026-11-20-r20b-stderr-fd-evidence-playbook.md`](../../reports/2026-11-20-r20b-stderr-fd-evidence-playbook.md). Sub-items (a–e) delivered in queue; follow-up work (if any) requires separate clarified/approved items.
