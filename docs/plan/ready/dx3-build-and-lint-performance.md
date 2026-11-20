# DX3a — CI/repo compiler caching integration (build and lint performance)

- **ID:** dx3-build-and-lint-performance
- **Status:** ready (approved)
- **Created:** 2026-11-20
- **Updated:** 2026-11-20
- **Source/links:**
  - [`../2026-11-11-developer-tooling-dx-improvement-plan.md`](../2026-11-11-developer-tooling-dx-improvement-plan.md) (DX3, P1; local `IDF_CCACHE_ENABLE` + ccache ~1.94× warm clean-build speedup measured 2026-11-11; Makefile `SOURCES` deterministic discovery done 2026-11-12; repo/CI compiler-cache integration and lint-pipeline optimizations remain)
  - [`../../reports/2026-11-11-dx1-baseline-measurements.md`](../../reports/2026-11-11-dx1-baseline-measurements.md) (build/lint baselines, ccache comparison FB1–FB4)
  - [`../needs-clarification/developer-tooling-dx-improvement-plan.md`](../needs-clarification/developer-tooling-dx-improvement-plan.md) (umbrella triage; DX3 selected 2026-11-20, per-slice explicit approval required before move to ready)
  - [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (canonical tracker)
  - CI/workflows: `.github/workflows/build-and-lint.yml`, `.github/workflows/integration-test.yml`, `Makefile`, `README.md`
- **Approval:** user (2026-11-20, clarify-feature conversation) — explicit approval to mark **ready (approved)** and move to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.

## Summary
First DX3 slice (**DX3a**): integrate/validate compiler caching (ccache / `IDF_CCACHE_ENABLE` as appropriate) for ESP-IDF firmware builds in CI workflows to reduce redundant compilation time and resource use, with correct cache keys/invalidation, preserved required CI coverage/signal, and documented local usage. Based on DX1 evidence showing ~1.94× warm clean-build speedup locally with ccache.

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
- [ ] CI workflows updated for compiler caching in scope: `.github/workflows/build-and-lint.yml` and `.github/workflows/integration-test.yml` (integration firmware builds) enable/configure `ccache`/`IDF_CCACHE_ENABLE` as appropriate, with `actions/cache` configured for compiler cache dirs and cache keys/restore-keys/invalidation approach aligned to relevant build inputs (including container/image/toolchain context). Existing `integration_test/test_*/build` caching remains consistent where applicable.
- [ ] Required CI checks, coverage, and failure signal remain unchanged/preserved (no reduction in lint/test/build gates or diagnostics needed for required checks).
- [ ] Documentation updated for local compiler-cache usage and CI caching behavior/limitations (README and/or relevant `docs/**`), including actionable env/tooling notes where practical.
- [ ] Evidence recorded in PR/docs: CI run ID(s)/links with required checks green and cache integration in effect (cache steps/behavior noted when visible). Timing deltas/cache-hit notes included when feasible (DX1 baseline/report context) or explicitly documented as blocked/unavailable with reason.
- [ ] Local checks for touched scope executed and reported per Validation plan (or blockers documented with actionable steps per `AGENTS.md`); no unrun checks claimed as passed.

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
- **Environment differences:** local ccache behavior and paths differ across Windows/Linux/macOS; docs must state practical/local notes and limitations.
- **Multi-platform claims:** do not claim cross-platform validation unless executed; mark unvalidated where applicable.

## Open questions
- None.

## Progress / evidence
- 2026-11-20: created from DX umbrella/source plan/DX1 report after user selected DX3 as next DX slice. Clarification started.
- 2026-11-20 (clarify-feature, user): selected **DX3a (CI/repo compiler caching integration)** as first slice; recorded future changed-files lint policy preference as **advisory local-only** (out of DX3a scope); set evidence baseline (CI/workflow evidence + local docs/commands; timing deltas when feasible). Accepted suggested DX3a defaults (scope, cache design, docs, DoD/evidence, validation plan, areas/files, dependencies/risks). **Open questions cleared; spec complete.**
- 2026-11-20 (clarify-feature, user approval): explicit approval recorded to mark **ready (approved)** and move from `docs/plan/needs-clarification/` to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human. No implementation started.