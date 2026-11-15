# R15–R17 — Experimental bytecode, UI allocation policy, solidification

- **ID:** r15-r16-r17-experimental-bytecode-ui-solidification
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R15/R16/R17, P2, not started), [`history/2026-11-04-after-ts-optimizations.md`](../../history/2026-11-04-after-ts-optimizations.md), related ideas: [`ideas/2026-11-09-new-ideas.md`](../../ideas/2026-11-09-new-ideas.md) (R24 bytecode persistence proposal)
- **Approval:** pending

## Summary
Separate, benchmark-gated experiments: R15 optional `.bec` bytecode pipeline/loader, R16 native UI allocation benchmarking/policy changes, R17 Berry solidification evaluation (firmware-bundled only). Guardrails require measurement, versioning, and no general dynamic TS app solidification.

## Open questions
- [ ] Which experiment (R15/R16/R17 or R24 overlap) should be tackled first, and can they be split into independent ready items?
- [ ] **R15 vs R24:** reconcile scope (optional pipeline/loader vs opt-in `.bec` persistence with freshness/fallback across load paths) and define the approved design/config mechanism.
- [ ] **R15:** what benchmarks/acceptance criteria (startup time, heap/flash impact, version-tracking, fallback/corruption handling) are required before implementation?
- [ ] **R16:** what workloads/metrics (retention vs fragmentation, peak/largest block, responsiveness) and success thresholds justify policy changes?
- [ ] **R17:** which firmware-bundled modules are candidates, and what memory/maintenance tradeoffs and build/tooling constraints are acceptable?
- [ ] Target hardware/environment and evidence format for each experiment?

## Acceptance criteria
- [ ] Each selected experiment has a clarified, approved spec/design, benchmarks defined, and becomes its own ready item(s) with validation plan and evidence requirements.

## Validation plan
- Automated: `make format-check`, `make test`, firmware build/lint as applicable.
- Hardware/manual: required for meaningful memory/runtime benchmarks on target.

## Progress / evidence
- 2026-11-15: created (not started; design/benchmark decisions required).
