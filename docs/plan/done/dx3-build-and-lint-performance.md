# DX3a — CI/repo compiler caching integration (build and lint performance)

- **ID:** dx3-build-and-lint-performance
- **Status:** done (delivered, pending human review/approval and merge)
- **Created:** 2026-11-20
- **Updated:** 2026-11-20
- **Source/links:**
  - [`../2026-11-11-developer-tooling-dx-improvement-plan.md`](../2026-11-11-developer-tooling-dx-improvement-plan.md) (DX3, P1; local `IDF_CCACHE_ENABLE` + ccache ~1.94× warm clean-build speedup measured 2026-11-11; Makefile `SOURCES` deterministic discovery done 2026-11-12)
  - [`../../reports/2026-11-11-dx1-baseline-measurements.md`](../../reports/2026-11-11-dx1-baseline-measurements.md) (build/lint baselines, ccache comparison FB1–FB4, DX3a addendum)
  - [`../needs-clarification/developer-tooling-dx-improvement-plan.md`](../needs-clarification/developer-tooling-dx-improvement-plan.md) (umbrella triage; DX3a selected 2026-11-20)
  - [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (canonical tracker)
  - CI/workflows: `.github/workflows/build-and-lint.yml`, `.github/workflows/integration-test.yml`, `Makefile`, `README.md`
  - Delivery PR: (to be opened, branch `docs/dx3a-ci-compiler-caching`)
- **Approval:** user (2026-11-20, clarify-feature conversation) — explicit approval to mark **ready (approved)** and move to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.

## Summary
First DX3 slice (**DX3a**): integrate/validate compiler caching (ccache / `IDF_CCACHE_ENABLE` as appropriate) for ESP-IDF firmware builds in CI workflows to reduce redundant compilation time and resource use, with correct cache keys/invalidation, preserved required CI coverage/signal, and documented local usage. Scoped workflow integration (`.github/workflows/build-and-lint.yml`, `.github/workflows/integration-test.yml`) was already present on `main`; this delivery finalizes documentation, evidence recording, and plan/queue state.

## Scope (in scope — DX3a)
- Add/validate compiler caching for ESP-IDF firmware builds in `.github/workflows/build-and-lint.yml` and `.github/workflows/integration-test.yml` (integration firmware builds), using `ccache` / `IDF_CCACHE_ENABLE` as appropriate for the ESP-IDF v6.1 CI environment.
- Configure GitHub Actions `actions/cache` for compiler cache directories used by CI builds (and keep existing `integration_test/test_*/build` caching consistent where applicable). Define cache keys based on runner/OS, container/image/toolchain identifiers, and hashes of inputs affecting builds (workflow files, CMake/IDF project config, `sdkconfig.defaults`/relevant config, lockfiles/scripts and other relevant inputs as applicable) with sensible `restore-keys`; document invalidation/update approach. Ensure ccache availability/configuration in CI image/steps as needed.
- Preserve all required CI checks, coverage, and failure signal (no lint/test scope reduction, no weakening of gates).
- Document local compiler-cache usage (`IDF_CCACHE_ENABLE`, ccache install/path notes per platform where practical) and CI caching behavior/limitations in `README.md` and/or relevant docs (`docs/**`, including DX/report notes as appropriate).
- Record evidence: CI workflow/run IDs (and required checks green) with observed cache behavior noted when visible; include timing deltas/cache-hit notes when feasible (leveraging DX1 baseline/report context), but document as blocked/unavailable if not measurable in the environment.

## Non-goals (out of scope — DX3a)
- Lint pipeline reuse, changed-files/impacted-files clang-tidy mode, and clang-tidy cache/compile DB scoping optimizations (separate DX3 slice(s); policy for future changed-files lint recorded below).
- Lint output wrappers/summaries (SARIF/JUnit/problem-matcher summaries) without losing raw diagnostics (separate slice if pursued).
- Unrelated parallel-build ergonomics or behavior changes not required for compiler-cache integration.
- Changing firmware/runtime behavior, test semantics, or required CI coverage/signal.

## Decisions (recorded 2026-11-20, clarify-feature, user)
- **First slice:** **DX3a — CI/repo compiler caching integration** selected (Recommended). Split remaining DX3 work (lint reuse/changed-files mode, lint summaries, parallel-build ergonomics/reconfigure avoidance as separate concerns) into future independent slices as needed.
- **Changed-files lint policy (future DX3b/lint scope, not in DX3a):** **Advisory local-only check (not a required gate).** If/when introduced, full lint remains the required CI/pre-push gate as today.
- **Evidence baseline:** **CI/workflow evidence + local docs/commands; timing deltas when feasible** (Recommended). Timing deltas/cache-hit notes included when feasible (DX1 context) but may be documented as blocked/unavailable if not measurable.
- **DX3a defaults accepted (2026-11-20):** scope, cache design, documentation, DoD/evidence, validation plan, expected areas/files, and dependencies/risks as specified in this document.

## Acceptance criteria / Definition of done (DX3a)
- [x] CI workflows updated for compiler caching in scope: `.github/workflows/build-and-lint.yml` and `.github/workflows/integration-test.yml` (integration firmware builds) enable/configure `ccache`/`IDF_CCACHE_ENABLE` as appropriate, with `actions/cache` configured for compiler cache dirs and cache keys/restore-keys/invalidation approach aligned to relevant build inputs (including container/image/toolchain context). Existing `integration_test/test_*/build` caching remains consistent where applicable. (Verified on `main`/`2658d03` and in CI run evidence.)
- [x] Required CI checks, coverage, and failure signal remain unchanged/preserved (no reduction in lint/test/build gates or diagnostics needed for required checks). (No gate/scope weakening in scoped workflows; required aggregate jobs preserved.)
- [x] Documentation updated for local compiler-cache usage and CI caching behavior/limitations (README and/or relevant `docs/**`), including actionable env/tooling notes where practical. (`README.md`, `docs/reports/2026-11-11-dx1-baseline-measurements.md` DX3a addendum, DX plan docs updated.)
- [x] Evidence recorded in PR/docs: CI run ID(s)/links with required checks green and cache integration in effect (cache steps/behavior noted when visible). Timing deltas/cache-hit notes included when feasible (DX1 baseline/report context) or explicitly documented as blocked/unavailable with reason.
- [x] Local checks for touched scope executed and reported per Validation plan (or blockers documented with actionable steps per `AGENTS.md`); no unrun checks claimed as passed.

## Validation plan
- **Local (as feasible, report exact outcomes):**
  - `make format-check`
  - `make test`
  - `make docs-check` (docs touched)
  - `make build` and/or `make lint` in ESP-IDF v6.1 environment if available; if unavailable or blocked, record concrete blocker (tooling/env/cause) and rely on PR CI evidence for ESP-IDF/clang-tidy validation.
- **CI (primary evidence for workflow/ESP-IDF behavior):** PR/branch workflow run(s) demonstrating required checks pass with compiler caching integrated in scoped workflows; record run ID(s)/links and note cache behavior/hits where visible. Capture timing deltas when feasible; otherwise document measurement limitation/blocker.
- **Reporting:** do not claim checks or hardware validation that were not executed. Follow `AGENTS.md` definition of done and global gates.

## Expected areas/files
- `.github/workflows/build-and-lint.yml`
- `.github/workflows/integration-test.yml`
- `README.md` and/or relevant docs under `docs/` (including DX/report notes if updated)
- `Makefile` and/or scripts only if minimally required for cache enablement/configuration (not expected to change behavior)

## Dependencies / prerequisites
- ESP-IDF v6.1 CI container/image provides or can install/configure `ccache` (or equivalent supported mechanism) for the relevant build tools; workflow permissions allow `actions/cache` usage.
- GitHub Actions cache availability/limits and stable cache key strategy based on relevant inputs.
- Access to CI runs (PR/branch) to collect evidence (run IDs/green checks/cache behavior). Local ESP-IDF v6.1 environment needed only for optional local build/lint verification; blockers must be documented if unavailable.

## Risks / constraints
- **Cache correctness/invalidation:** stale cache entries if keys omit relevant inputs can cause confusing build/lint behavior; keys must include relevant toolchain/config/source/workflow inputs and be maintainable.
- **Cache effectiveness/limits:** GitHub Actions cache size/eviction and runner image differences may reduce hit rates; timing improvements may vary and are “when feasible” to measure.
- **No coverage/signal regression:** caching must not skip or alter required checks, diagnostics, or failure conditions.
- **Workflow overlap/merge conflicts:** `unblock-shell-fs-pipeline-and-qemu-hangs` (ready) may also require evidence-backed changes to `.github/workflows/integration-test.yml` and/or integration/QEMU config; coordinate sequencing/rebase to avoid conflicting workflow edits while preserving required checks and coverage.
- **Environment differences:** local ccache behavior and paths differ across Windows/Linux/macOS; docs must state practical/local notes and limitations.
- **Multi-platform claims:** do not claim cross-platform validation unless executed; mark unvalidated where applicable.

## Open questions
- None.

## Progress / evidence
- 2026-11-20: created from DX umbrella/source plan/DX1 report after user selected DX3 as next DX slice. Clarification started.
- 2026-11-20 (clarify-feature, user): selected **DX3a (CI/repo compiler caching integration)** as first slice; recorded future changed-files lint policy preference as **advisory local-only** (out of DX3a scope); set evidence baseline (CI/workflow evidence + local docs/commands; timing deltas when feasible). Accepted suggested DX3a defaults (scope, cache design, docs, DoD/evidence, validation plan, areas/files, dependencies/risks). **Open questions cleared; spec complete.**
- 2026-11-20 (clarify-feature, user approval): explicit approval recorded to mark **ready (approved)** and move from `docs/plan/needs-clarification/` to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human. No implementation started.
- 2026-11-20 (Ponytail Audit, plan-conflict resolution, doc-only): added coordination/risk note for possible `.github/workflows/integration-test.yml` overlap with `unblock-shell-fs-pipeline-and-qemu-hangs` (ready). No implementation/validation performed.
- 2026-11-20 (implement-next-feature, docs/delivery pass): verified scoped workflow implementation already present on `main` (`2658d03`): `.github/workflows/build-and-lint.yml` and `.github/workflows/integration-test.yml` enable `IDF_CCACHE_ENABLE=1`/`CCACHE_DIR`, install `ccache`, restore/save `.ccache` via `actions/cache` with runner/OS+`idf-v6.1` and build-input-hash keys (per-suite for integration), run `ccache --zero-stats`/`--show-stats`, and preserve existing `integration_test/test_*/build` caching and required CI gates/coverage. Updated `README.md` and appended DX3a addendum to `docs/reports/2026-11-11-dx1-baseline-measurements.md`; updated DX plan docs/cross-links.
- **Pre-PR CI evidence on `main` (required checks green, cache integration in effect):**
  - CI [44559762592](https://github.com/gallayl/cos/actions/runs/44559762592) (success, 2026-11-19, head `4ef50c0470c561803003a6f13cd87cf6cf2f4540`, PR #94 merge): `lint / ESP-IDF Build & Lint` and all 8 `integration-tests / Integration Tests (...)` matrix jobs executed successfully with ccache install/restore/build/stats/save steps. Sampled build-and-lint logs showed cache restored from restore-key, `ccache --zero-stats`/`--show-stats` ran (`CCACHE_ENABLE=True`), cache saved; sampled stats window reported 0% hits / 1334 misses for that build (hit rate varies by key/history).
  - CI [44691115573](https://github.com/gallayl/cos/actions/runs/44691115573) (success, 2026-11-20, head `fb458adea5e97f3398ae11a15abc2eb5705a2ad5`, PR #98 merge): scoped build-and-lint and integration-test jobs executed successfully with ccache install/restore/build/stats/save steps; sampled logs confirmed restore-key use and ccache stats/enablement.
  - Main branch health baseline at start: CI [44742729257](https://github.com/gallayl/cos/actions/runs/44742729257) (success, head `2658d03`, docs-only path-filtered run; scoped ESP-IDF jobs skipped as not applicable).
- **Timing deltas/cache-hit notes:** DX1 local baseline remains ~139.5 s clean → ~72.1 s warm cached clean rebuild (~1.94×, 73.7% hit rate, 2026-11-11). CI timing deltas and controlled cache-hit comparisons **not measured** in this DX3a delivery pass (environment/cache-history/path-filter variability); cache steps/behavior observed as noted above when scoped jobs ran.
- **Local validation (this branch, docs-scope delivery):** `make format-check` — pass (exit 0). `make test` — pass (exit 0). `make docs-check` — pass (exit 0). `make build` / `make lint` — **not executed** (ESP-IDF v6.1 toolchain/QEMU not invoked in this docs-only pass; no firmware/workflow-source behavior changes in this PR). Blocker/remediation: run in a sourced ESP-IDF v6.1 environment (`IDF_PATH`/tools/python env, PATH to `idf.py`/cmake/ninja/toolchains per README) with `make build` and `make lint` (or rely on PR CI build-and-lint/integration jobs) and record exact outcomes. No unrun checks claimed as passed.
- **PR/CI (this delivery):** PR to be opened (branch `docs/dx3a-ci-compiler-caching`). CI run/link to be recorded after PR creation and checks complete (required checks must be green per scope; scoped ccache steps/logs will be visible only if build-and-lint/integration jobs execute).
- **Independent review (substantive docs/plan delivery):** pending (will run `code-review`/reviewer loop after PR opened/updated and resolve actionable findings with re-validation as needed).
- **Remaining human actions:** review/approval and merge (squash expected). Required hardware/manual validation: none for this docs/CI-config evidence delivery (no firmware/runtime behavior changes). If a human wants fresh on-PR ccache log visibility despite path filters, trigger/run scoped jobs on a branch/run that executes build-and-lint/integration workflows and record results (optional follow-up, not a blocker for this docs-scope delivery).
