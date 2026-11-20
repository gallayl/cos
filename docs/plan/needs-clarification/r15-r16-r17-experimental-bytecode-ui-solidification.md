# R15–R17 — Experimental bytecode, UI allocation policy, solidification

- **ID:** r15-r16-r17-experimental-bytecode-ui-solidification
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R15/R16/R17, P2, not started), [`history/2026-11-04-after-ts-optimizations.md`](../../history/2026-11-04-after-ts-optimizations.md), related ideas: [`ideas/2026-11-09-new-ideas.md`](../../ideas/2026-11-09-new-ideas.md) (R24 bytecode persistence proposal)
- **Approval:** pending (clarification decisions/defaults recorded 2026-11-20; no implementation approval)

## Summary
Separate, benchmark-gated experiments: R15 optional `.bec` bytecode pipeline/loader, R16 native UI allocation benchmarking/policy changes, R17 Berry solidification evaluation (firmware-bundled only). Guardrails require measurement, versioning, and no general dynamic TS app solidification.

## Decisions (recorded 2026-11-20, clarify-feature, user-accepted suggested defaults)
- **Prioritization/split:** **Prioritize R15 first**; treat R15/R16/R17 as **separate, benchmark-gated experiments** and split into independent ready items before implementation.
- **R15 vs R24 reconciliation:** **Unify R15/R24 into one reconciled bytecode design when pursued.** R15 scoped as optional `.bec` pipeline/loader experiment with source path remaining supported. Reconcile with R24 before implementation (opt-in persistence/caching, freshness, fallback, load-path behavior, config/versioning) — no divergent implementations.
- **R15 benchmarks/acceptance (default baseline):** Target hardware/environment: **ESP32-2432S028R (CYD), ESP-IDF v6.1** (record firmware image/commit/config). Compare source vs bytecode on representative shipped scripts/apps (e.g. autoexec, a command script, a UI app where feasible). Measure startup/load time, heap free/largest block/min watermark during load/run, and flash/storage impact. Require bytecode **version/compatibility tracking** and **automatic safe fallback** on mismatch/corruption/read failure; **no correctness/behavior regression**. `.bec` remains optional. Evidence: reproducible commands/scripts, before/after measurements, and pass/fail conclusions. Detailed per-slice thresholds must be explicit before implementation (may be refined when split to ready).
- **R16 (default baseline):** Workloads = repeated UI create/update/destroy and app open/close cycles (representative screens/widgets). Metrics = peak heap, largest block, min watermark, fragmentation indicators, and responsiveness/stalls if observable. Success = no progressive leak and no material regression (or measured improvement) in memory/behavior/responsiveness with evidence on target hardware.
- **R17 (default baseline):** Candidates limited to **selected firmware-bundled/stable modules only** (never general dynamic TS app deployment). Evaluate memory/flash/startup and maintenance/build/tooling tradeoffs with an explicit candidate list before adoption.
- **Target hardware/environment & evidence (default baseline):** Use target ESP32/CYD hardware where runtime/memory behavior matters (record env/firmware/commit/config). Evidence in markdown report(s) under `docs/reports/` with environment/commit, exact steps, raw before/after numbers, and pass/fail conclusions. Split each experiment into its own ready item with finalized AC/validation before implementation.

## Open questions
- None (user-accepted defaults recorded 2026-11-20). **Not blocked by unanswered user questions.**

## Blockers / unblock conditions
- **Current state:** clarified/deferred pending active scoping. When pursued, create independent, fully specified, explicitly approved ready item(s) per experiment (starting with R15) with finalized benchmarks/thresholds, scope/non-goals, validation, dependencies/risks, and expected areas/files per Ready contract. Hardware/manual benchmarking required for meaningful evidence.

## Acceptance criteria
- [ ] Each selected experiment has a clarified, approved spec/design and benchmarks defined (refining defaults as needed) and becomes its own ready item(s) with validation plan and evidence requirements before implementation.

## Validation plan
- Automated: `make format-check`, `make test`, firmware build/lint and other scope-appropriate checks as applicable (`AGENTS.md`).
- Hardware/manual: required for meaningful memory/runtime benchmarks on target (CYD/ESP32) with recorded evidence.

## Progress / evidence
- 2026-11-15: created (not started; design/benchmark decisions required).
- 2026-11-20 (clarify-feature, user): recorded prioritization (R15 first, split experiments), R15/R24 unified reconciliation intent, and user-accepted suggested defaults for R15 benchmarks/acceptance, R16/R17 criteria, and target HW/evidence format. Open questions cleared; item remains needs-clarification/deferred pending active scoping and split into ready item(s) (not blocked by unanswered user questions).
