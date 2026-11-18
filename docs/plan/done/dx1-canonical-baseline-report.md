# DX1 — Canonical baseline measurements and CI evidence report

- **ID:** dx1-canonical-baseline-report
- **Status:** done (delivered)
- **Created:** 2026-11-17
- **Delivered:** 2026-11-18
- **Source/links:** [`docs/plan/needs-clarification/developer-tooling-dx-improvement-plan.md`](../needs-clarification/developer-tooling-dx-improvement-plan.md) (triage umbrella, first selected slice), [`docs/plan/2026-11-11-developer-tooling-dx-improvement-plan.md`](../2026-11-11-developer-tooling-dx-improvement-plan.md) (DX1 source), [`docs/reports/2026-11-11-dx1-baseline-measurements.md`](../../reports/2026-11-11-dx1-baseline-measurements.md) (updated baseline report), [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md), [`docs/README.md`](../../README.md), root [`AGENTS.md`](../../../AGENTS.md), workflows: `.github/workflows/build-and-lint.yml`, `.github/workflows/test.yml`, `.github/workflows/frontend.yml`, `.github/workflows/format-check.yml`, `.github/workflows/docs-check.yml`, `.github/workflows/integration-test.yml`, PR [#86](https://github.com/gallayl/cos/pull/86)
- **Approval:** **Approved 2026-11-17 by user (explicit, in-session):** mark `ready` and move to `docs/plan/ready/`. **Pre-approved for implementation/PR flow** (merge remains a human/maintainer action). Delivered in PR [#86](https://github.com/gallayl/cos/pull/86) (PR CI green, 2026-11-18); pending human review/approval and merge.

## Summary
Complete and record the remaining **DX1** baseline/evidence gaps selected for this slice by updating the existing DX1 report with lint and integration build/run baseline measurements plus comparable GitHub Actions CI timing/evidence, captured/compared in the defined canonical environment model. This is a docs/evidence deliverable to enable evidence-based DX performance/CI follow-ups.

## Decisions recorded (2026-11-17, user)
- **Prioritization context:** DX1 baselines/evidence first (umbrella decision).
- **Slice scope (in):** (1) **Lint baseline** — full `make lint` baseline (timing, pass/fail, env/toolchain/compile-DB prep notes) in canonical local env when available; if not runnable, record concrete blocker including attempted command and missing prerequisites (satisfies lint sub-scope). No changed-files mode required. (2) **Integration build/run baselines** — **all 6** integration projects; record **build baselines and run baselines where feasible** using canonical flows in ESP-IDF v6.1 + QEMU env (serial default unless specified), include parallelism notes/`INTEGRATION_TEST_JOBS` if used; if run not feasible, record build results plus concrete run blockers. (3) **CI runner timing comparison** — cite **latest representative (prefer successful) GitHub Actions runs on `main`** for `build-and-lint` and `integration-test` workflows (and others only if clearly relevant), recording run IDs/URLs, runner/image assumptions, and comparable job/step timings from logs/artifacts; no re-trigger required if evidence is sufficient. Unrunnable/unavailable parts must be recorded as concrete blockers/unvalidated (not claimed passes).
- **Canonical environment model:** **Local ESP-IDF v6.1 + GitHub Actions CI comparison** — record local measurements in an ESP-IDF v6.1-capable environment when available and compare with GitHub Actions (ubuntu-latest/container as used) using existing CI runs/logs; document assumptions and mark anything not run as unvalidated/blocked.
- **Delivery/report target:** **Update existing** `docs/reports/2026-11-11-dx1-baseline-measurements.md` (not a new dated report file) and update plan/queue links as applicable.
- **Validation for this docs/evidence slice:** run/record `make docs-check` (and `make format-check` if affected) per `AGENTS.md`; do not claim unrun build/lint/integration measurements.
- **Finalization:** user confirmed draft acceptance criteria, expected areas/files, dependencies/prerequisites, and risks/constraints as acceptable with out-of-scope boundaries below (2026-11-17).

## Scope and non-goals

### In scope
- Update `docs/reports/2026-11-11-dx1-baseline-measurements.md` to include:
  - **Lint baseline:** full `make lint` command(s), compile DB preparation/prerequisite notes (e.g. `scripts/prepare_lint_db.py`, `build/clang/compile_commands.json` freshness as relevant), environment/toolchain details, wall-clock timing, pass/fail outcome, and notable diagnostics summary or blockers if not runnable (including attempted command and missing prerequisites).
  - **Integration build/run baselines (all 6 projects):** canonical command(s) (e.g. `scripts/integration_test.py`, `make integration-test-build`, `make integration-test-run`/`make integration-test`), ESP-IDF v6.1/QEMU/Python dependency environment details (`integration_test/requirements.txt` context), projects covered, parallelism settings/notes (`INTEGRATION_TEST_JOBS` if used), build and run timings/outcomes and artifact/log notes where feasible; if run not feasible, record build results plus concrete run blockers.
  - **CI runner timing comparison:** latest representative (prefer successful) GitHub Actions runs on `main` for `build-and-lint` and `integration-test` workflows (and others only if clearly relevant), with run IDs/URLs or identifiable references, runner/image assumptions, comparable job/step timings and outcomes from logs/artifacts, and comparison notes vs local measurements.
- Record dates, environment/cache-state assumptions where relevant, and explicit **unvalidated/blocked** items for anything not executed (per `AGENTS.md`: never claim unrun checks).
- Keep cross-links/status consistent: this item, [`developer-tooling-dx-improvement-plan.md`](../needs-clarification/developer-tooling-dx-improvement-plan.md), and [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) as applicable.

### Out of scope (non-goals)
- Implementing performance improvements (DX3/DX4/DX7), environment/bootstrap changes (DX2), output/UX/task-runner work (DX5/DX6), or adding new timing/summary tooling/automation beyond what is needed to record this report.
- **Cross-platform (Linux/macOS) baseline measurements**, **peak RAM/disk/log-volume metrics**, except incidental notes if already available (must be marked unvalidated if not measured).
- Hardware/on-device validation.
- Removing historical evidence from existing reports. Re-triggering CI runs when existing run evidence is sufficient. Claims for checks/measurements not executed.

## Open questions
None.

## Acceptance criteria / definition of done
- [x] `docs/reports/2026-11-11-dx1-baseline-measurements.md` updated with in-scope **lint baseline** (full `make lint` attempted locally with command/timing/outcome/blocker details; compile DB present), **integration build/run baseline** evidence (all 8 discovered integration projects covered by CI execution evidence; local build/run not executed due to explicit toolchain/QEMU blockers recorded), and **CI runner timing comparison** (latest representative/prefer successful `main` runs including CI [44332595802](https://github.com/gallayl/cos/actions/runs/44332595802) with lint/build and all 8 integration suites, CI [44376892335](https://github.com/gallayl/cos/actions/runs/44376892335), and standalone workflow runs [44084418948](https://github.com/gallayl/cos/actions/runs/44084418948)/[44084418967](https://github.com/gallayl/cos/actions/runs/44084418967) with path-filter-skipped heavy jobs noted) — including commands, timings, dates, environment/runner assumptions, comparison notes, and explicit blockers/unvalidated notes where execution was not possible. (Spec requested 6 projects; report documents 8 present and covered.)
- [x] No unrun checks/measurements claimed as executed/passed; unavailable env measurements recorded as blocked/unvalidated with actionable detail.
- [x] Plan/queue cross-links kept consistent: this item (moved to `docs/plan/done/` on delivery), [`developer-tooling-dx-improvement-plan.md`](../needs-clarification/developer-tooling-dx-improvement-plan.md), and consolidated tracker cross-links as applicable (no DX1-specific tracker entry required).
- [x] Required docs-scope validation for this change recorded per `AGENTS.md`: `make format-check` **pass** (exit 0), `make docs-check` **pass** (exit 0), `make test` **pass** (exit 0, 30/30 CTest + 3 Python unittest tests). `make lint` was executed but **failed** in local Windows env (exit 2, ~74309 ms) and is not claimed as passing.
- [x] **Open questions: none** and explicit approval recorded; implementation/reporting delivered in PR [#86](https://github.com/gallayl/cos/pull/86) with PR CI green (2026-11-18). Pending human review/approval and merge. No hardware/manual validation required for this docs/evidence slice.
## Validation plan
- **This clarification/spec drafting pass (docs-only):** run and record `make docs-check` (and `make format-check` if affected by these doc edits) as feasible per `AGENTS.md`; do not claim unrun checks.
- **Implementation of this ready item (when approved/executed):** collect and record in-scope measurements/evidence in `docs/reports/2026-11-11-dx1-baseline-measurements.md`:
  - **Lint (local, ESP-IDF v6.1 env when available):** run `make lint` (document compile DB/prep, e.g. `scripts/prepare_lint_db.py` / `build/clang/compile_commands.json`, and toolchain). Record exact command(s), timing, outcome, environment. If unrunnable, record concrete blocker with attempted command and missing prerequisites.
  - **Integration (local, ESP-IDF v6.1 + QEMU when available):** for **all 6 projects**, run canonical build baseline (e.g. `make integration-test-build` and/or `scripts/integration_test.py build ...`) and run baseline where feasible (e.g. `make integration-test-run` / `make integration-test` and/or `scripts/integration_test.py run ...`), noting parallelism/`INTEGRATION_TEST_JOBS` if used. Record exact commands, timings, outcomes, artifacts/log notes. If run not feasible, record build results plus concrete run blockers.
  - **CI comparison:** gather timings/outcomes from latest representative (prefer successful) `main` runs of `.github/workflows/build-and-lint.yml` and `.github/workflows/integration-test.yml` (and others only if clearly relevant) via Actions logs/artifacts; record run IDs/URLs, runner/image assumptions, and comparable job/step timings. No re-trigger required if evidence sufficient.
  - Record dates, environments/cache-state assumptions, results (pass/fail/timing), and blockers/unvalidated items. Do not claim hardware validation. Do not claim checks run if tooling/env/CI access unavailable.
- **Cross-platform:** Linux/macOS measurements out of scope; mark unvalidated if not measured. Peak RAM/disk/log-volume metrics out of scope.

## Expected areas/files
- `docs/reports/2026-11-11-dx1-baseline-measurements.md` (primary evidence/report update)
- `docs/plan/done/dx1-canonical-baseline-report.md` (this item)
- `docs/plan/needs-clarification/developer-tooling-dx-improvement-plan.md` (umbrella triage cross-link/status)
- `docs/plan/consolidated-improvement-plan.md` (cross-link/status note if applicable)

## Dependencies/prerequisites
- ESP-IDF v6.1 environment and toolchain for local lint/integration measurements (as available).
- clang-tidy / ESP clang tools and prepared compile DB for `make lint` baseline (if run locally).
- QEMU (`qemu-system-xtensa`) and pinned Python deps in `integration_test/requirements.txt` for integration build/run baselines (if run).
- Access to GitHub Actions workflow run history/logs/artifacts for CI comparison.
- Time/resources for potentially long integration build/run measurements.

## Risks/constraints
- Timings are environment/cache-state dependent; results must include environment, cache/warm-cold state, and assumptions to be meaningful.
- CI log retention or access limits may constrain comparisons; record what was available.
- Integration build/run can be slow/resource-heavy; scope/parallelism must be explicit to keep measurements reproducible.
- Lint/integration commands may be blocked by missing local tooling/env; blockers must be recorded concretely without claiming success (per `AGENTS.md`).
- Cross-platform and peak-metric claims are out of scope for this slice; mark unvalidated if not measured.

## Progress / evidence
- 2026-11-17: created as first triage slice from `developer-tooling-dx-improvement-plan` (DX1 prioritized). User decisions recorded (scope gaps: lint, integration build/run, CI comparison; canonical: local ESP-IDF v6.1 + GitHub Actions comparison; delivery: update existing DX1 report; item ID confirmed).
- 2026-11-17 (clarify-feature, rounds 2–3): user confirmed lint/integration/CI specifics and finalized DoD/areas/files/deps/risks/out-of-scope. **Open questions set to none; spec complete.** User gave **explicit approval (2026-11-17, in-session)** to mark ready and move to `docs/plan/ready/dx1-canonical-baseline-report.md` (pre-approved for implementation/PR flow; merge remains human). Status set to **`ready` (approved)** and file moved. Implementation/reporting and required validation evidence remain to be completed.
- 2026-11-18 (implement-next-feature, branch `docs/dx1-canonical-baseline-report` from `origin/main` `fca4919`, baseline green evidence: CI run [44376892335](https://github.com/gallayl/cos/actions/runs/44376892335) success for `fca4919`): updated [`docs/reports/2026-11-11-dx1-baseline-measurements.md`](../../reports/2026-11-11-dx1-baseline-measurements.md) with DX1 completion pass — local lint baseline attempted (`make lint`, 74309 ms, exit 2, failed in this Windows env with clang-tidy errors/warnings despite `build/clang/compile_commands.json` present), local integration build/run baselines not executed/feasible (`qemu-system-xtensa` not on PATH, `idf.py` not on PATH; 8 integration projects enumerated and all covered by CI evidence), and CI runner timing comparison recorded (CI [44332595802](https://github.com/gallayl/cos/actions/runs/44332595802), head `039c663`, success: lint/build job + 8 integration suites with per-step timings; CI [44376892335](https://github.com/gallayl/cos/actions/runs/44376892335), head `fca4919`, docs-only skipped heavy jobs; standalone workflow runs [44084418948](https://github.com/gallayl/cos/actions/runs/44084418948)/[44084418967](https://github.com/gallayl/cos/actions/runs/44084418967), head `ed0032d0`, heavy jobs path-filter-skipped). Local validation recorded: `make format-check` **pass** (exit 0), `make docs-check` **pass** (exit 0), `make test` **pass** (exit 0, 30/30 CTest + 3 Python unittest tests). No unrun local measurements claimed as passed; blockers/unvalidated items recorded in report.
- 2026-11-18 (delivery): PR [#86](https://github.com/gallayl/cos/pull/86) opened (`docs(dx1): record canonical baseline measurements and CI evidence`, draft) and PR CI green (run [44378502751](https://github.com/gallayl/cos/actions/runs/44378502751), `CI Checks Required` success; heavy jobs path-filter-skipped as expected for docs-only change). Item moved from `docs/plan/ready/` to `docs/plan/done/` as **delivered** with recorded evidence. **Pending human actions:** review/approval and merge (human). No hardware/manual validation required for this docs/evidence slice. Local lint baseline remains a recorded failed attempt/blocker in this Windows env (not a passing claim); integration local runs remain blocked by missing QEMU/idf.py PATH in this env (CI evidence recorded).
