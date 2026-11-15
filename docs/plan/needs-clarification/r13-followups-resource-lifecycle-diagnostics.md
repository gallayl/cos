# R13 follow-ups — Resource lifecycle guarantees and memory-conscious diagnostics

- **ID:** r13-followups-resource-lifecycle-diagnostics
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R13-FU1, R13-FU2, optional TS contract), [`history/2026-11-04-after-ts-optimizations.md`](../../history/2026-11-04-after-ts-optimizations.md), [`reports/2026-11-09-r13-hardware-validation-guide.md`](../../reports/2026-11-09-r13-hardware-validation-guide.md)
- **Approval:** pending

## Summary
Broader R13 follow-ups: (FU1) native resource-cleanup registration/guarantees for timers/callbacks/listeners/handles beyond TS conventions, only with measurement/need; (FU2) configurable/granular memory-conscious diagnostics for hot callbacks/uncontrolled allocations with low false positives; optional stricter TS console/UI type split.

## Open questions
- [ ] Which follow-up (if any) is prioritized and what concrete problem/measurement motivates it?
- [ ] FU1: required cleanup guarantees, APIs, and measurable cases justifying native support?
- [ ] FU2: specific diagnostic rules, configurability, and acceptable signal/noise?
- [ ] Acceptance criteria and validation evidence for the chosen scope?

## Validation plan
- Automated: frontend/compiler tests, `make format-check`, `make test`, firmware build/lint/integration as applicable.
- Hardware/manual: as needed for lifecycle/resource behavior.

## Progress / evidence
- 2026-11-15: created (broader spec follow-ups; need prioritization/scope).
