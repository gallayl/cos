# R18 — `async`/`await` runtime design + implementation

- **ID:** r18-async-await-runtime
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R18, P2, not started), [`history/2026-11-07-ts-feature-extensions.md`](../../history/2026-11-07-ts-feature-extensions.md)
- **Approval:** pending

## Summary
Design-first effort: define concurrency model on constrained hardware (Berry VM single-threading/tasking, interaction with timers/network callbacks, cancellation, memory/resource lifetime) before any compiler lowering. No silent desugaring.

## Open questions
- [ ] What concurrency/execution model is approved (event loop integration, cooperative tasks/coroutines, callback bridging) given Berry VM and ESP32 constraints?
- [ ] Scope of first milestone (syntax subset, supported APIs like timers/fetch/UI callbacks, cancellation/error semantics)?
- [ ] Memory/stack/performance budgets and required measurements/benchmarks?
- [ ] How will async interact with existing `CosApplication`/`__cos_entry`/`run(args)` contracts and R13/R21 lifecycle rules?

## Acceptance criteria
- [ ] Runtime/concurrency design documented and approved.
- [ ] Minimal implementation + compiler support + tests delivered per approved design with evidence.

## Validation plan
- Automated: frontend/compiler tests, `make format-check`, `make test`, firmware build/lint/integration as applicable.
- Hardware/manual: required for runtime behavior/timing/resource validation.

## Progress / evidence
- 2026-11-15: created (design required before implementation).
