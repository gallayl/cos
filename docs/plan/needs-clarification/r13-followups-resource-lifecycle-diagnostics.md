# R13 follow-ups — Resource lifecycle guarantees and memory-conscious diagnostics

- **ID:** r13-followups-resource-lifecycle-diagnostics
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R13-FU1, R13-FU2, optional TS contract), [`history/2026-11-04-after-ts-optimizations.md`](../../history/2026-11-04-after-ts-optimizations.md), [`reports/2026-11-09-r13-hardware-validation-guide.md`](../../reports/2026-11-09-r13-hardware-validation-guide.md)
- **Approval:** pending (clarification decisions recorded 2026-11-20; no implementation approval)

## Summary
Broader R13 follow-ups: (FU1) native resource-cleanup registration/guarantees for timers/callbacks/listeners/handles beyond TS conventions, only with measurement/need; (FU2) configurable/granular memory-conscious diagnostics for hot callbacks/uncontrolled allocations with low false positives; optional stricter TS console/UI type split.

## Decisions (recorded 2026-11-20, clarify-feature, user)
- **Prioritization/motivation:** **No prioritization now — explicit deferral** (Recommended). No FU1/FU2/TS-split follow-up prioritized now (insufficient evidence/motivation or lower priority). Record unblock/motivation criteria for future reconsideration.
- **FU1 details:** **Deferred until prioritized/motivated.** Do not specify guarantees/APIs now; define when a concrete, measured need is identified.
- **FU2 details:** **Deferred until prioritized/motivated.** Do not specify rules/configurability/signal-noise now; define when concrete developer/runtime problems motivate them with acceptable false-positive tolerance.
- **Acceptance/validation (current state):** **Deferred — no implementation scope now.** Record unblock/motivation criteria; acceptance/validation will be defined per future prioritized slice when motivated by concrete evidence (measurements, repro cases, or developer need).

## Open questions
- None (user decisions recorded 2026-11-20). **Not blocked by unanswered user questions.**

## Blockers / unblock conditions
- **Current state:** explicitly deferred (no implementation scope approved). Revisit when motivated by concrete evidence: e.g. measured leak/crash/fragmentation or lifecycle failure justifying FU1, developer/runtime pain justifying FU2 with acceptable signal/noise, or clear need for stricter TS console/UI type split. When reprioritized, create/split a fully specified ready item with scope, AC, validation, deps/risks, and explicit approval.

## Validation plan
- Not applicable while deferred. Future prioritized slice: frontend/compiler tests, `make format-check`, `make test`, firmware build/lint/integration and hardware/manual as applicable per scope (`AGENTS.md`).

## Progress / evidence
- 2026-11-15: created (broader spec follow-ups; need prioritization/scope).
- 2026-11-20 (clarify-feature, user): recorded explicit deferral (no FU1/FU2/TS-split prioritized now) with details/acceptance deferred until motivated by concrete evidence. Open questions cleared; item remains needs-clarification as deferred/unprioritized (not blocked by unanswered user questions).
