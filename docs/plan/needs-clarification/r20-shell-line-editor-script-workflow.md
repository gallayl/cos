# R20c: Shell line-editor seam + on-device script workflow polish

- **ID:** r20-shell-line-editor-script-workflow
- **Status:** needs-clarification
- **Created:** 2026-11-18
- **Source/links:** docs/plan/needs-clarification/r20-deferred-backlog.md (umbrella, item c: shell line-editor refactor seam); docs/plan/done/r20-pipes-runtime-v1.md; docs/plan/2026-11-12-berry-command-script-migrations.md (R22); docs/plan/consolidated-improvement-plan.md (R20)
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
Clarify and deliver a small, practical improvement around the deferred shell line-editor refactor seam and day-to-day Berry command/script workflow on device. Goal: make editing/running/debugging scripts and migrated command scripts less friction-heavy (clear resolver/path feedback, basic script inspect/copy/help affordances) without attempting a full on-device graphical editor.

## Scope
- Define a concrete scope for the “line-editor refactor seam” follow-up in components/shell (what refactor seam is needed, boundaries, non-goals).
- Define practical on-device script workflow affordances (e.g. list/show resolved script path, inspect/copy stock vs user scripts patterns, clearer resolver failure messages, minimal help/examples) consistent with existing shell/file commands.
- Produce acceptance criteria and validation plan (host unit/integration as applicable, manual HW smoke for shell behavior).

## Non-goals
- Full on-device graphical/text editor UI.
- Redesigning the entire shell input stack beyond what the seam/scoped polish requires.
- Unscoped “do all of R20” work (separately tracked umbrella items remain elsewhere unless explicitly included).

## Acceptance criteria
- [ ] Seam/refactor scope documented with explicit boundaries and risk notes (ESP32 constraints).
- [ ] At least one practical script-workflow improvement is specified (commands/output behavior) and implemented only after approval/Ready transition.
- [ ] Automated checks per AGENTS.md for affected scope; manual shell/HW validation steps defined when hardware behavior matters.

## Validation plan
- Automated: make format-check, make test; add/update host tests in test/ or component tests as applicable; make build/lint/docs-check when scope requires (per AGENTS.md).
- Hardware/manual: required if shell line-editing behavior or device script resolution UX changes on CYD; record steps/results and blockers if unavailable.

## Expected areas/files
- components/shell/ (parsing/editor/runtime as scoped), test/, integration_test/ (if applicable), docs/reports or docs updates as needed.

## Dependencies / prerequisites
- R20 umbrella context (docs/plan/needs-clarification/r20-deferred-backlog.md).
- Alignment with R22 command-script resolver/path behavior (docs/plan/2026-11-12-berry-command-script-migrations.md).

## Risks / constraints
- ESP32 memory/stack and shell complexity; refactors risk regressions in input/editing behavior.
- Must not block on unrelated R20 sub-items unless explicitly scoped.

## Open questions
- [ ] What exact “line-editor refactor seam” is meant in current code (specific files/modules/symbols) and what minimal seam is sufficient?
- [ ] Which script-workflow affordances are in v1 (list/resolved path/inspect/copy/help) vs deferred?
- [ ] Any constraints from Berry command script resolver (R9/R22) that must be preserved?
- [ ] What manual HW test matrix is required (keyboard/touch/serial as applicable)?

## Progress / evidence
- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 1).
