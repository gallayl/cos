# Developer Tooling & DX Improvement Plan

**Created:** 2026-11-11  
**Status:** Proposed plan (not started)  
**Scope:** Local developer experience, dependency/setup workflow, build/lint/integration-test performance, log/output ergonomics, and CI parity for the COS monorepo (ESP-IDF firmware, host tests, QEMU integration tests, frontend, docs).  
**Companion docs:** [`docs/plan/consolidated-improvement-plan.md`](./consolidated-improvement-plan.md) (product/runtime/memory/compiler backlog), [`docs/README.md`](../README.md), root [`README.md`](../../README.md), [`AGENTS.md`](../../AGENTS.md).

> This plan addresses cross-cutting engineering productivity pain points identified in a 2026-11-11 project-structure review. It is intentionally focused on tooling/DX; runtime, firmware feature, and memory-pressure items remain tracked in the consolidated plan unless explicitly linked.

## Goals

- Reduce first-time and recurring environment/setup effort on Windows, Linux, and macOS.
- Cut inner-loop and CI wall-clock time / resource consumption for build, lint, host tests, and QEMU integration tests without reducing correctness guarantees.
- Make failures actionable by default: concise summaries first, full diagnostics available on demand or in artifacts.
- Keep local verification aligned with CI (`AGENTS.md`, `.github/workflows/*`, Make targets) and preserve multi-platform support (including USB flashing constraints on native hosts).

## Non-goals

- Changing firmware behavior, Berry runtime semantics, or product features except where required to support tooling (tracked separately, e.g. R2/R5/R8/R9/R13+ in consolidated plan).
- Replacing ESP-IDF/QEMU/pytest-embedded with non-equivalent test infrastructure.
- Mandating a single OS or IDE; recommendations must remain viable across Windows/Linux/macOS.

## Pain points and root causes (evidence)

| Pain point | Evidence / likely contributors |
|---|---|
| Dependency installation is overcomplicated | Multiple toolchains/runtimes: ESP-IDF v6.1 + env vars (`IDF_PATH`, tools/python env), host GCC/CMake/Ninja/Python, clang-format/clang-tidy, QEMU + pinned `pytest-embedded-*`, Node 24 + Yarn for frontend (`README.md`, `.github/workflows/*`, `integration_test/requirements.txt`, `frontend/package.json`). Makefile injects tool paths but setup remains env-heavy (`Makefile:3-29`). Python entrypoints vary (`IDF_PYTHON`, system `python`, pytest). Generated/tracked artifact ambiguity increases confusion (`.gitignore` ignores `sdkconfig`/`build`/`managed_components`/`frontend/dist`/`frontend/node_modules`, but such artifacts exist in workspace snapshots). |
| Build/lint/integration testing slow and resource-heavy | No repo/CI-configured compiler cache (`ccache`/`sccache`/launcher). Separate clang reconfigure DB (`scripts/prepare_lint_db.py` → `build/clang`) plus full clang-tidy over explicit `SOURCES` list (`Makefile:30-109`, `scripts/run_tidy.py`). 6 independent integration firmware projects (`integration_test/test_*`) built serially by default and run serially with verbose pytest (`scripts/integration_test.py` supports `-j` build but Makefile/CI do not use it by default). Host tests reconfigure/build each run (`Makefile:110-117`). Frontend toolchain is dependency-heavy on cold runs. |
| Output too noisy | Verbose defaults: raw `idf.py` output, command echoing, clang-tidy streaming all diagnostics, `pytest -v`, integration logs/artifacts/backtrace decoding in CI (`.github/workflows/integration-test.yml`, `scripts/integration_test.py`, `scripts/run_tidy.py`). Useful for forensics, but obscures first actionable failure in normal loops. |
| Maintainability friction | Manual `SOURCES` enumeration in `Makefile:30-70` risks drift when components/files change. Environment definitions are spread across README, Makefile, devcontainer, and CI workflows. |

## Principles

- **Measure before tuning:** record baseline wall-clock, CPU, peak RAM/disk, and log volume for representative workflows before/after changes.
- **Fast inner loop, full CI gate:** optimize local/PR iteration (caching, parallelism, scoped checks) while retaining full coverage in CI/nightly/pre-merge as appropriate.
- **Summary-first diagnostics:** default output should surface the first/root failure and a short summary; full logs remain accessible (`--verbose`, log files, CI artifacts/groups).
- **Single blessed environment, portable escape hatches:** provide one reproducible setup path (container/devcontainer + scripts) and keep documented native workflows for hardware access and platform-specific needs.
- **Multi-platform by default:** every new script/tool must work on Windows + Linux (and macOS where feasible), or ship platform-specific equivalents with identical behavior.

## Workstreams and prioritized actions

### DX1 — Baseline metrics and observability (P0, prerequisite)

**Objective:** make performance/noise improvements evidence-based and prevent regressions.

- **Actions**
  - [ ] Define canonical benchmark scenarios: clean + incremental `make build`, `make lint`, `make test`, `make integration-test-build`, `make integration-test-run` (all + single project), frontend `yarn install/build/test/lint`.
  - [ ] Capture baseline metrics per platform available (time, parallelism, cache state, artifact sizes, log line/byte counts, notable peak memory where measurable).
  - [ ] Add a lightweight way to collect timings/summaries (script or Make targets) and store results in `docs/reports/` when measured.
  - [ ] Document CI runner/resource assumptions (e.g. ubuntu-latest, container image) alongside results.
- **Acceptance:** baseline report checked in or linked; future DX changes cite before/after deltas.
- **Risks/notes:** hardware-adjacent steps excluded; focus on host/QEMU/frontend/docs checks.

### DX2 — Reproducible environment & dependency simplification (P0)

**Objective:** one obvious, supported path to a working toolchain; fewer manual env vars.

- **Actions**
  - [ ] Promote `.devcontainer` (`espressif/idf:v6.1`-based) as the **canonical** full-stack firmware/integration environment; document VS Code Dev Containers usage for Windows/Linux/macOS and native Docker/Podman caveats.
  - [ ] Add/refresh bootstrap scripts for native setups (PowerShell + Bash) that: validate required tools/versions, locate or install guidance for ESP-IDF/QEMU/clang tools, create/activate the expected Python env, and print/export only the minimal required variables.
  - [ ] Standardize Python workflow for repo tooling/tests with a managed project environment (recommend `uv` for speed/lock support, or `pip-tools` + venv as fallback), including pinned `integration_test/requirements.txt` and script-test deps; document which commands must run under IDF Python vs repo-managed Python.
  - [ ] Align frontend toolchain setup: document Corepack/Yarn version pinning, `yarn install --immutable/--frozen-lockfile`, Node 24.x requirement, and cache locations; ensure CI and local commands match.
  - [ ] Audit and resolve generated/tracked artifact policy (`sdkconfig`, `managed_components`, `frontend/dist`, build outputs, caches): either stop tracking generated outputs, or document why committed and add freshness checks.
  - [ ] Reduce env-var burden where possible: improve auto-detection in scripts/Make targets, provide a single checked-in example env template (`.env.example` or documented `direnv` snippet) without committing secrets.
- **Acceptance:** a new contributor can reach "build + host test + selected integration test" via documented canonical path with minimal manual variable setup; native path documented with explicit prerequisites.
- **Risks/notes:** USB serial passthrough differs across hosts/containers; native flashing workflow must remain first-class.

### DX3 — Build and lint performance (P1)

**Objective:** reduce redundant compilation/configuration and speed static analysis.

- **Actions**
  - [ ] Enable compiler caching for supported compilers/builds (e.g. `CCACHE`/`IDF_CCACHE_ENABLE`/CMake compiler launcher, `sccache` where preferable), with documented cache dirs per OS and CI cache integration.
  - [ ] Ensure parallel builds are easy and explicit (`idf.py -j`/Ninja parallelism defaults or documented flags); avoid unnecessary clean/reconfigure steps in common targets.
  - [ ] Review lint pipeline for reuse: avoid redundant `reconfigure` when inputs unchanged, cache/scope compile DB generation if practical, and support **changed-files/impacted-files** clang-tidy mode for local iteration while keeping full lint in CI (or as a required pre-push check).
  - [ ] Replace manual `SOURCES` glob maintenance in `Makefile` with deterministic discovery (e.g. from tracked files/components or compile DB) for format/lint coverage; keep behavior identical in CI.
  - [ ] Consider optional incremental/problem-matcher-friendly lint wrappers (SARIF/JUnit summaries) without losing raw diagnostics.
- **Acceptance:** measurable reduction in incremental/clean build and lint times on at least one reference platform; format/lint coverage automatically includes new component sources; CI remains green/equivalent.
- **Risks/notes:** clang-tidy against embedded toolchains can be sensitive to flags/DB freshness; validate `scripts/run_tidy.py` sanitization behavior after changes.

### DX4 — Test and integration-test throughput (P1)

**Objective:** make host and QEMU validation loops practical for frequent local runs.

- **Actions**
  - [x] Expose parallel integration firmware builds in `Makefile`/docs (e.g. `INTEGRATION_TEST_BUILD_ARGS=-jN` or dedicated target). (2026-11-11: added `INTEGRATION_TEST_JOBS` support to `make integration-test-build` and documented usage in `README.md`; safe default concurrency for developer machines/CI still to evaluate.)
  - [ ] Evaluate safe default concurrency for developer machines/CI.
  - [ ] Evaluate safe parallelism for integration test execution (independent QEMU instances, resource caps); keep serial mode available and default where isolation/races are a concern.
  - [ ] Add targeted selection workflows as first-class UX (single project, glob/pattern, changed-area mapping) using existing `scripts/integration_test.py` selectors.
  - [ ] Improve host-test loop ergonomics: avoid redundant configuration when unchanged, support running subsets, and keep `ctest --output-on-failure` summaries clear.
  - [ ] Investigate (strategic, may defer): shared caches/artifacts across integration projects, consolidation/parameterization opportunities only if coverage and debuggability are preserved (link to consolidated-plan follow-up if adopted).
- **Acceptance:** documented fast paths for "one test project" and "all projects" with improved wall-clock time; no loss of required CI coverage.
- **Risks/notes:** QEMU and ESP-IDF builds can be RAM/CPU intensive when parallelized; provide caps and fallback guidance.

### DX5 — Output ergonomics and failure signal (P1)

**Objective:** default output answers "what failed and why?" quickly.

- **Actions**
  - [ ] Introduce consistent verbosity levels/modes across wrappers (`quiet`/default/`verbose`): print concise phase summaries and first actionable errors by default; write full logs to files (and CI artifacts) when useful.
  - [ ] Add CI log grouping/annotations where supported (e.g. GitHub Actions groups) and prefer summary tables for pass/fail per target/project.
  - [ ] Tune test runners for signal: e.g. `pytest -q --tb=short` default with rerun/verbose-on-failure option; keep clang-tidy/compiler diagnostics complete but summarize counts/first error.
  - [ ] Standardize command echoing to a single, copy-pastable line and suppress repetitive progress noise where safe.
  - [ ] Ensure scripts return meaningful exit codes and distinguish "environment/tool missing" from "check failed" with actionable remediation hints (aligned with `AGENTS.md` reporting guidance).
- **Acceptance:** typical failure cases (format violation, unit test failure, single integration failure, lint error) identifiable from the last ~50 lines of default output; full detail still available.
- **Risks/notes:** avoid hiding intermittent/flaky context needed for embedded/QEMU debugging.

### DX6 — Developer workflow UX and guardrails (P2)

**Objective:** make the right checks easy to run early.

- **Actions**
  - [ ] Add a thin cross-platform task runner interface (e.g. `just` or Taskfile) wrapping existing Make/scripts with common presets: `setup-check`, `check-fast`, `check-ci`, `lint-changed`, `itest <name> [-jN]`, `logs-verbose`.
  - [ ] Introduce optional fast pre-commit hooks (format/whitespace/basic lint) via `pre-commit` or equivalent, kept quick to avoid resistance.
  - [ ] Document "inner loop vs full gate" guidance in `README.md`/`docs/` and link from `AGENTS.md` expectations as needed.
  - [ ] Add editor-agnostic config hygiene where helpful (`.editorconfig`, consistent line endings, problem matchers/tasks) without forcing a specific IDE.
- **Acceptance:** common workflows invokable via memorable commands on all supported platforms; optional hooks improve catch rate without blocking normal commits.
- **Risks/notes:** keep Make targets canonical for CI to avoid drift.

### DX7 — CI efficiency and parity (P2)

**Objective:** faster, clearer CI with the same guarantees.

- **Actions**
  - [ ] Add caching to workflows: pip/uv, frontend package manager store, ESP-IDF/QEMU tools where feasible, and compiler caches keyed by toolchain/config hashes.
  - [ ] Use parallel integration builds/runs where runner resources and test isolation allow; upload summarized results and retain detailed logs/artifacts only as needed.
  - [ ] Align CI invocation with new quiet/summary modes while preserving artifact richness on failure.
  - [ ] Periodically review container/toolchain pin freshness and startup cost (image size/layers, preinstalled tools) in coordination with ESP-IDF version policy.
- **Acceptance:** reduced median CI duration and/or queue-to-signal time for affected workflows, with unchanged or clearer failure reporting.
- **Risks/notes:** cache correctness/invalidation must be handled carefully for embedded toolchains.

## Suggested execution order

1. **DX1** — establish baselines and reporting format.
2. **DX2** — simplify/reproducify environments (largest onboarding/setup leverage; enables consistent measurement).
3. **DX3 + DX4** — attack build/lint/test throughput (caching, parallelism, scoping, selection).
4. **DX5** — reduce noise once fast paths exist (summary-first UX, CI grouping).
5. **DX6 + DX7** — workflow polish and CI efficiency/parity.

Quick wins to prioritize early (can start in parallel with DX1 instrumentation): expose integration parallel build flags in docs/Make, add CI/frontend/pip caching, add summary/quiet wrappers, and auto-discover format/lint sources.

## Success metrics (track per platform/workflow)

- Setup: time from clean machine/container to successful `make test` + selected integration test; number of manual env steps.
- Performance: wall-clock for clean/incremental build, full lint, host tests, all integration builds/runs, single integration project; CI job durations.
- Resources: peak CPU/RAM/disk during parallel integration builds (where measurable), cache hit rates (if reported).
- Signal quality: default log volume (lines/bytes) for success/failure runs; time-to-first-actionable-error in sampled failures.
- Coverage safety: no regression in required checks; format/lint source discovery stays complete.

## Validation expectations

- For each implemented DX change, run the relevant checks per `AGENTS.md` (at minimum affected Make/Yarn targets and CI-equivalent commands when feasible) and report command/result evidence.
- Do not claim hardware-dependent validation; QEMU/host/frontend/docs checks should be automated where possible.
- Cross-platform claims require verification or explicit "not yet validated on <OS>" notes.

## Open questions / decisions to record during implementation

- Canonical Python manager for repo tooling (`uv` vs alternatives) and interaction with `IDF_PYTHON`.
- Whether/where `sdkconfig`, `managed_components`, and `frontend/dist` should be source-controlled or generated (and freshness enforcement strategy).
- Safe default parallelism for integration builds/tests on CI runners vs developer machines.
- Scope of "changed-files lint" as advisory local check vs required gate.
- Feasibility of consolidating integration firmware projects without reducing behavioral coverage (strategic, evidence required).

## Progress tracker

| ID | Workstream | Priority | Status | Notes |
|---|---|---:|---|---|
| DX1 | Baseline metrics and observability | P0 | 🟡 In progress | 2026-11-11: initial local script/tool baselines + firmware build baseline and ccache comparison recorded in `docs/reports/2026-11-11-dx1-baseline-measurements.md` (FB1–FB4: clean 139.52 s → warm cached 72.09 s, −48.3%); lint/integration/CI/cross-platform/peak metrics remain. |
| DX2 | Reproducible environment & dependency simplification | P0 | ⬜ Not started |  |
| DX3 | Build and lint performance | P1 | 🟡 In progress | 2026-11-11: validated local compiler caching via `IDF_CCACHE_ENABLE=1` + ccache (Espressif tools) with ~1.94× warm clean-build speedup and 73.7% hit rate; repo/CI integration and lint-pipeline optimizations remain. |
| DX4 | Test and integration-test throughput | P1 | 🟡 In progress | 2026-11-11: exposed `INTEGRATION_TEST_JOBS` in Makefile/docs (parallel build flag quick win); safe defaults/parallel run/selection ergonomics remain. |
| DX5 | Output ergonomics and failure signal | P1 | ⬜ Not started |  |
| DX6 | Developer workflow UX and guardrails | P2 | ⬜ Not started |  |
| DX7 | CI efficiency and parity | P2 | 🟡 In progress | 2026-11-11: local evidence supports adding compiler caching to CI (build-and-lint/integration workflows); implementation/validation in progress as next step. |
