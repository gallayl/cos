# DX1 Baseline Measurements (developer tooling / scripts)

**Date:** 2026-11-11  
**Platform:** Windows (win32), workspace `E:\dev\cos`  
**Branch/status at capture:** `feat/r14-ts-diags...origin/feat/r14-ts-diags [ahead 1]` with local modifications in `.devcontainer/devcontainer.json`, `.vscode/settings.json` (pre-existing; not changed for this baseline)  
**Context:** Initial DX1 baseline pass per `docs/plan/2026-11-11-developer-tooling-dx-improvement-plan.md` (session task: start with baseline measurements, run scripts, measure execution time, track progress).

## Environment snapshot (available tools)

| Tool / check | Result |
|---|---|
| `make` | `c:\Program Files (x86)\GnuWin32\bin\make.exe` |
| `cmake` | Present (WinLibs) |
| `gcc` (host) | Present (WinLibs POSIX UCRT) |
| `python` | 3.14 (`C:\Users\gallayl\AppData\Local\Programs\Python\Python314\python.exe`) |
| `node` | `C:\Program Files\nodejs\node.exe` |
| `yarn` | `C:\Users\gallayl\AppData\Roaming\npm\yarn.ps1` (v1.22.22 observed in runs) |
| `clang-format` | Present (`...\Python314\Scripts\clang-format.exe`) |
| `clang-tidy` | `C:\Espressif\tools\esp-clang\esp-21.1.3_20260408\esp-clang\bin\clang-tidy.exe` |
| `idf.py` on PATH | Not found as direct command in this shell |
| `qemu-system-xtensa` on PATH | Not found |
| `berry` compiler executable | Not found (no `berry.exe`/`berry` discovered; only test binaries named `*berry*` under `test/build`) |
| ESP-IDF env vars (shell) | `IDF_PATH=C:/esp/v6.1/esp-idf/`, `IDF_TOOLS_PATH=C:/Espressif/tools`, `IDF_PYTHON_ENV_PATH=C:/Espressif/tools/python/v6.1/venv` observed; `idf.py` exists at `$IDF_PATH/tools/idf.py` |
| `frontend/node_modules` | Present |
| `build/` (firmware) | Present (existing artifacts) |
| `pytest` | 9.2.0 (`python -m pytest --version`) |

## Measured script/command baselines

Wall-clock times below are from `System.Diagnostics.Stopwatch` around the invoked command in this environment (single runs, warm/cold state as noted by context: existing `build/` and `frontend/node_modules` present; `make test` reported `ninja: no work to do`).

| ID | Command / script | Exit | Time (ms) | Result / notes |
|---:|---|---:|---:|---|
| B1 | `.\scripts\validate-berry-scripts.ps1 -SkipIfCompilerMissing` | 0 | 265 | **SKIP (warning):** Berry compiler not found. Script handled missing compiler path as expected with skip flag. Full validation (without skip) not run: no host Berry compiler located. |
| B2 | `make format-check` | 0 | 807 | Passed. Runs `clang-format --dry-run --Werror` over Makefile `SOURCES`. |
| B3 | `python scripts/integration_test.py build --help` | 0 | 206 | Passed (CLI/help path). Script entrypoint and argument parsing verified. |
| B4 | `make test` | 0 | 1908 | Passed. CMake configure/generate fast, Ninja `no work to do`, CTest **24/24 passed** (real ~0.25s) plus `python -m unittest discover -s scripts -p "test_*.py"` (**3 tests, 0.018s, OK**). |
| B5 | `frontend`: `yarn format:check` | 0 | 1534 | Passed (`prettier --check .`, "All matched files use Prettier code style!", ~1.29s reported by yarn). |
| B6 | `frontend`: `yarn lint` | 0 | 8245 | Passed (`eslint .`, ~8.00s reported by yarn). |
| B7 | `frontend`: `yarn test` | 0 | 2841 | Passed (`vitest run`: **19 files, 132 tests passed**, ~2.24s vitest / ~2.63s yarn). |
| B8 | `frontend`: `yarn build` | 0 | 5534 | Passed (`tsc && vite build`, built in ~2.60s / ~5.32s yarn). Vite warning emitted (large chunks >500 kB), not a failure. |

**Aggregate (measured commands B1–B8):** all executed commands exited 0 in this pass; total measured wall time ~21.34s (sum of individual stopwatch timings; sequential execution).

## Firmware build baseline and compiler-caching comparison (local, 2026-11-11 follow-up)

**Environment/state:** same Windows host (`E:\dev\cos`, ESP-IDF v6.1 tools, `ccache` 4.12.1 at `C:\Espressif\tools\ccache\4.12.1\ccache-4.12.1-windows-x86_64\ccache.exe`). The pre-existing `build/` CMake cache originated from `/workspaces/cos` (devcontainer path) and caused `make build` to fail until `build/` was removed; measurements below were taken after recreating a valid local build.

**How caching was enabled (local only):** `IDF_CCACHE_ENABLE=1` with the ccache directory prepended to `PATH` (ESP-IDF/CMake reported `-DCCACHE_ENABLE=True` on cached runs; baseline reported `-DCCACHE_ENABLE=False`). No repository or CI files were changed for this measurement.

| Scenario | Compiler cache | Exit | Time (ms) | Time (s) | Notes |
|---|---:|---:|---:|---:|---|
| **FB1 (baseline):** clean `make build` (after `build/` removed) | off (`IDF_CCACHE_ENABLE=0`) | 0 | 139520 | 139.52 | Full firmware build completed (`cos.bin` 0x1d9bc0, 7% free in smallest app partition). |
| **FB2:** `make clean` + `make build` | on (`IDF_CCACHE_ENABLE=1`) | 0 / 0 | 29445 + 85931 | 29.45 + 85.93 | First ccache-enabled build; populates compiler cache. |
| **FB3 (warm):** `make clean` + `make build` | on | 0 / 0 | 72093 (total) | 72.09 | Clean rebuild with warm ccache (primary comparison to FB1). |
| **FB4:** incremental no-op `make build` | on | 0 | 1961 | 1.96 | Ninja up-to-date. |

**`ccache -s` snapshot (after FB1–FB4):** cacheable 10564/10599 (99.67%), hits 7782/10564 (**73.67%**, direct 5166 / preprocessed 2616), misses 2782/10564 (26.33%), local cache size 0.1/5.0 GiB.

**Comparison vs baseline (FB1 = 139.52 s, clean build):**
- Warm cached clean rebuild (FB3): **72.09 s** → **−67.43 s (−48.3%)**, **~1.94× faster**.
- First/populating cached clean build (FB2): **85.93 s** → **−53.59 s (−38.4%)**, **~1.62× faster**.

**Conclusion:** compiler caching (`IDF_CCACHE_ENABLE` + `ccache`) provides a substantial local speedup for clean/rebuild cycles on this setup and is worth introducing to CI (with tool availability, `CCACHE_DIR`, cache restore/save keyed by toolchain/IDF/config, and hit-rate/size monitoring). CI integration and cross-platform validation remain follow-up work.

## Scripts/commands identified but not fully baseline-measured

| Script / command | Status | Reason / blocker | Suggested follow-up |
|---|---|---|---|
| `.\scripts\validate-berry-scripts.ps1` (without `-SkipIfCompilerMissing`) | Not measured | Berry host compiler not found in environment (`BERRY_EXE`/build locations/PATH). | Build pinned host Berry compiler from `build/berry-source` per `components/berry_lang/CMakeLists.txt`, set `BERRY_EXE` or pass `-BerryCompiler`, then time full `.be` validation (default `storage/flash_data`). |
| `python scripts/integration_test.py build [all\|<project>]` | Not measured (local, 2026-11-11) | Full integration builds are ESP-IDF/QEMU-env dependent and potentially long/resource-heavy (6 projects in spec context); not executed in this baseline pass to avoid uncontrolled duration. ESP-IDF appears installed but full activated env/idf.py invocation not validated here. | Run in ESP-IDF v6.1 environment (e.g. devcontainer or activated shell). Capture clean+incremental timings, with `-j`/`INTEGRATION_TEST_JOBS` comparisons (DX4). |
| `python scripts/integration_test.py run [all\|<project>]` | Not measured (local, 2026-11-11) | Requires built QEMU firmware images, `pytest-embedded` deps, and QEMU (`qemu-system-xtensa` not on PATH in this shell). | Measure per-project and full-suite after successful build; record env/cache state. |
| `.\scripts\r14-hardware-validation.ps1` | Not executed (by design) | Hardware/firmware validation helper; creates artifact directories/checklists and may run `idf.py` in `-FirmwareEval` mode. Not a generic software-only baseline script. | Use only during R14 hardware validation with target device/serial; time artifact-prep step separately if desired. |
| `make lint` | Attempted locally 2026-11-18 (failed in this env; see update section) | Earlier 2026-11-11 pass did not run it. 2026-11-18 pass executed full `make lint` with compile DB present but encountered clang-tidy/compile-environment errors (missing headers in analyzed translation units) and exited non-zero. | Re-run in a fully aligned ESP-IDF v6.1 + clang toolchain environment (e.g. devcontainer/CI image) with fresh compile DB; capture clean timing and pass/fail diagnostics. |
| `make build` (firmware) | **Partially measured (local, 2026-11-11)** | Clean build baseline and ccache comparison captured in follow-up (FB1–FB4). | Capture incremental-after-change, lint-coupled, and CI/devcontainer timings; validate on Linux/macOS. |
| `make integration-test` | Not measured (local, 2026-11-11/2026-11-18) | Integration orchestration depends on activated ESP-IDF toolchain and QEMU; high cost and QEMU unavailable locally in 2026-11-18 pass. | Baseline in canonical env with clean/incremental runs. |
| Frontend `yarn install` / cold-cache scenarios | Not measured | `node_modules` already present. | Capture clean install (`--frozen-lockfile`) and cache-state variants separately. |

## 2026-11-18 DX1 completion pass (lint, integration build/run, CI comparison)

**Date:** 2026-11-18  
**Branch/status at capture:** `docs/dx1-canonical-baseline-report` (created from `origin/main` `fca491961b42f4ec8bc98c049544d04b13e2c1a0`, docs-only working changes for this DX1 slice)  
**Canonical environment model (per spec):** local ESP-IDF v6.1-capable environment when available + GitHub Actions CI comparison. **Out of scope/unvalidated in this pass:** cross-platform (Linux/macOS) measurements, peak RAM/disk/log-volume metrics.

### 2026-11-18 local environment snapshot

| Tool / check | Result / notes |
|---|---|
| Platform | Windows (`win32`), workspace `E:\dev\cos` |
| `make` | GNU Make 3.81 (`make --version`) |
| `node` | v24.10.0 |
| `python` | not re-probed in this pass (see 2026-11-11 snapshot) |
| `clang-tidy` | `C:\Espressif\tools\esp-clang\esp-21.1.3_20260408\esp-clang\bin\clang-tidy.exe` (`where.exe clang-tidy`) |
| `idf.py` on PATH | Not found (`where.exe idf.py` / direct invocation unavailable) |
| `qemu-system-xtensa` on PATH | Not found |
| ESP-IDF env vars | `IDF_PATH=C:/esp/v6.1/esp-idf/`, `IDF_TOOLS_PATH=C:/Espressif/tools`, `IDF_PYTHON_ENV_PATH=C:/Espressif/tools/python/v6.1/venv` |
| Compile DB (lint) | Present: `build/clang/compile_commands.json` and `build/compile_commands.json` (freshness not independently proven beyond presence) |
| `integration_test/` projects discovered (`test_*` dirs) | **8 projects present (repo state 2026-11-18):** `test_esp_littlefs_core`, `test_esp_littlefs_extra`, `test_init_sequence`, `test_nvs_calibration`, `test_shell_fs`, `test_stability`, `test_system`, `test_vfs_ops` (**note:** DX1 slice text stated "all 6"; evidence below covers all 8 discovered projects and CI-executed suites). |

### Local lint baseline (attempted, canonical local env check)

- **Command:** `make lint`
- **Environment/context:** Windows host, ESP-IDF env vars set, clang-tidy available, `build/clang/compile_commands.json` present (compile DB preparation/freshness steps not re-run in this pass; no `scripts/prepare_lint_db.py` invocation recorded).
- **Wall-clock time:** 74309 ms (~74.31 s), measured via `System.Diagnostics.Stopwatch` around `make lint`.
- **Exit code:** 2 (`make: *** [lint] Error 1`)
- **Outcome:** **failed** in this local environment. clang-tidy processed 112 files and reported a very large warning volume plus errors (summary observed near completion: `315396 warnings and 11 errors generated` overall; multiple `clang-diagnostic-error` cases such as missing headers in specific translation units, e.g. `light_sensor.h`, `system.h`, `boot_profile.h`, `service_diagnostics.h`, `service_lifecycle.h`, `bluetooth.h`, and additional analyzer warnings). This indicates the local lint/compile-database/toolchain alignment was not sufficient for a clean/passing full lint run in this environment.
- **Not claimed:** no passing lint result and no clean warning/error baseline claim for this environment.

### Local integration build/run baselines (attempted/assessed; runs blocked)

- **Canonical commands (repo):** `make integration-test-build`, `make integration-test-run`, `make integration-test` (Makefile wrappers over `scripts/integration_test.py build|run`; `INTEGRATION_TEST_JOBS`/`-j` supported for build parallelism). Direct CLI also available via `python scripts/integration_test.py build [all|<project>]` and `python scripts/integration_test.py run [all|<project>]`.
- **Projects in scope (spec stated all 6; actual repo count 8):** `test_esp_littlefs_core`, `test_esp_littlefs_extra`, `test_init_sequence`, `test_nvs_calibration`, `test_shell_fs`, `test_stability`, `test_system`, `test_vfs_ops`.
- **Local build baselines:** **not executed** in 2026-11-18 pass. Attempted prerequisite assessment: `idf.py` not directly invocable on PATH (IDF env vars present) and full local ESP-IDF/QEMU integration execution path not validated in-session. No per-project build timing/outcome recorded locally.
- **Local run baselines:** **not feasible in this environment**. Concrete blocker: `qemu-system-xtensa` not found on PATH; run flow requires built QEMU firmware images, pytest-embedded/QEMU setup, and pinned deps in `integration_test/requirements.txt` in an activated ESP-IDF/QEMU-capable environment. No run timing/outcome recorded locally.
- **Parallelism notes:** not exercised locally (`INTEGRATION_TEST_JOBS` not set/recorded for local execution).
- **Artifacts/logs:** none produced locally for integration build/run in this pass.

### CI runner timing comparison (GitHub Actions, `main`)

**Runner/image assumptions (from workflow definitions and observed runs):** `ci.yaml` orchestrates reusable workflows on `push`/`pull_request` to `main`; reusable jobs observed on `ubuntu-latest` for change-detection/required gates and in `espressif/idf:v6.1` containers for ESP-IDF build/lint and integration QEMU jobs (see workflow files: `.github/workflows/ci.yaml`, `.github/workflows/build-and-lint.yml`, `.github/workflows/integration-test.yml`). CI uses path filters (`dorny/paths-filter`) so heavy jobs may be legitimately skipped when not applicable.

#### A) Latest representative successful consolidated CI run (relevant `main` context)

- **Run:** CI [44376892335](https://github.com/gallayl/cos/actions/runs/44376892335) — **success** (completed 2026-11-18T19:06:36Z), event `push`, branch `main`, head `fca491961b42f4ec8bc98c049544d04b13e2c1a0` ("docs: inventory pending hardware validations and console command plans (#84)")
- **Outcome/timings:** created 19:06:12Z, updated 19:06:36Z (wall ~24s). All required gates succeeded; build/lint, frontend, unit tests, and integration test matrix jobs were **skipped** as not applicable by path filters (docs-only change). Useful as latest successful `main` CI evidence for gate behavior, but not comparable for build/lint/integration execution timings.

#### B) Latest representative successful `main` CI run with actual lint/build and integration execution (all 8 suites)

- **Run:** CI [44332595802](https://github.com/gallayl/cos/actions/runs/44332595802) — **success** (completed 2026-11-18T16:36:24Z), event `push`, branch `main`, head `039c66328441aaea319489c83f3cafb0437aaa67` ("feat(http): add M1 upload memory measurement instrumentation and report (#78)")
- **Overall timing:** created 15:10:48Z, updated 16:36:24Z (wall ~1h25m36s). `CI Checks Required` succeeded.

**Lint/build (reusable `build-and-lint` path via CI):**
- `lint / ESP-IDF Build & Lint` (job [134953185543](https://github.com/gallayl/cos/actions/runs/44332595802/job/134953185543)): 15:11:00Z–15:18:58Z (~7m58s), **success**.
  - Key steps: Initialize containers 57s (15:11:02–15:11:59), Install ESP-IDF clang tools and ccache 58s, Restore ccache 19s, **Build firmware 2m49s** (15:13:18–15:16:07), **Generate Clang compile database 20s** (15:16:07–15:16:27), **Run clang-tidy 1m49s** (15:16:27–15:18:16), Post Restore ccache 36s.
- `lint / Required`: 15:19:01Z–15:19:04Z, success.

**Integration tests (reusable `integration-test` path via CI, 8 suites executed, all success):**
- `test_esp_littlefs_extra` (job [134953185473](https://github.com/gallayl/cos/actions/runs/44332595802/job/134953185473)): 15:11:00–15:18:41 (~7m41s). Build 18s (15:13:59–15:14:17), Run 4m08s (15:14:18–15:18:26).
- `test_init_sequence` (job [134953185799](https://github.com/gallayl/cos/actions/runs/44332595802/job/134953185799)): 15:11:00–15:13:32 (~2m32s). Build 43s, Run 5s.
- `test_nvs_calibration` (job [134953186011](https://github.com/gallayl/cos/actions/runs/44332595802/job/134953186011)): 15:11:01–15:15:29 (~4m28s). Build 27s, Run 5s.
- `test_stability` (job [134953186301](https://github.com/gallayl/cos/actions/runs/44332595802/job/134953186301)): 15:11:00–15:14:49 (~3m49s). Build 31s, Run 1m21s.
- `test_vfs_ops` (job [134953186865](https://github.com/gallayl/cos/actions/runs/44332595802/job/134953186865)): 15:11:01–15:14:26 (~3m25s). Build 23s, Run 32s.
- `test_system` (job [134953187119](https://github.com/gallayl/cos/actions/runs/44332595802/job/134953187119)): 15:11:01–15:14:10 (~3m09s). Build 35s, Run 20s.
- `test_esp_littlefs_core` (job [134953288964](https://github.com/gallayl/cos/actions/runs/44332595802/job/134953288964)): 15:11:01–15:17:56 (~6m55s). Build 23s, Run 4m52s.
- `test_shell_fs` (job [134953183576](https://github.com/gallayl/cos/actions/runs/44332595802/job/134953183576)): 16:26:57–16:36:11 (~9m14s; **late start relative to other suites in the same workflow run** — timing reported as observed). Build 31s (16:28:28–16:28:59), Run 6m34s (16:29:02–16:35:36).
- `integration-tests / Required`: 16:36:14–16:36:17, success.

**Other CI jobs (for context):** `test / Unit Tests` 15:10:58–15:11:16 (18s, success), `format-check / Format Check` 15:11:01–15:11:13 (12s, success), frontend changes gate required/no frontend heavy run in this path-filtered context as recorded.

#### C) Latest successful standalone runs of requested workflow files on `main` (path-filter-skipped heavy jobs)

- **Build & Lint workflow:** [44084418948](https://github.com/gallayl/cos/actions/runs/44084418948) — **success** (2026-11-17T12:34:31Z–12:34:50Z), head `ed0032d078691b1da3ab43ca76e2f8286e859c65` ("ci: Try fix fi action (#65)"). Jobs: `changes` success (7s), `ESP-IDF Build & Lint` **skipped** (path filter), `Required` success. **No build/lint execution timing available** from this run.
- **Integration Tests workflow:** [44084418967](https://github.com/gallayl/cos/actions/runs/44084418967) — **success** (2026-11-17T12:34:31Z–12:34:54Z), head `ed0032d078691b1da3ab43ca76e2f8286e859c65`. Jobs: `changes` success, `Integration Tests (${{ matrix.suite }})` **skipped**, `Required` success. **No integration build/run execution timing available** from this run.

### Comparison notes (local vs CI)

- **Lint:** local full `make lint` attempt failed (exit 2, ~74.3s) in this Windows environment despite present compile DB/clang-tidy; CI run 44332595802 completed lint/build job successfully in `espressif/idf:v6.1` container with firmware build + clang DB generation + clang-tidy in ~7m58s job time (including container/setup/ccache restore/save). Results are **not directly comparable** due to environment/toolchain differences and local failure mode.
- **Integration build/run:** local baselines not executed (QEMU/tooling blockers). CI run 44332595802 executed all 8 integration projects successfully with per-suite build/run step timings reported above (cache restores/build dirs/pip cache present in steps). CI parallelism/scheduling and cache state differ from a cold local run and from `INTEGRATION_TEST_JOBS`-controlled local build parallelism.
- **General:** timings are environment-, cache-, and path-filter-dependent; CI evidence is taken from logs/metadata as available (no re-trigger performed).

## Progress / tracking

- [x] Located DX task context: `docs/plan/2026-11-11-developer-tooling-dx-improvement-plan.md` (DX1 baseline/observability).
- [x] Inventoried environment and runnable scripts/tooling.
- [x] Ran and timed quick/safe script baselines (Berry validation skip, integration script help, format/test/lint/build commands available locally).
- [x] Ran and timed host tests (`make test`) and frontend CI-equivalent scripts (`yarn format:check`, `yarn lint`, `yarn test`, `yarn build`).
- [x] Recorded results in this report: `docs/reports/2026-11-11-dx1-baseline-measurements.md`.
- [x] Captured local firmware `make build` baseline and ccache comparison (FB1–FB4): clean no-cache 139.52 s, warm cached clean rebuild 72.09 s (−48.3%, ~1.94×), 73.67% ccache hits.
- [x] Full Berry compile validation baseline (needs host Berry compiler). *(unchanged from 2026-11-11: not executed; host Berry compiler unavailable — remains a blocker.)*
- [x] Integration test build/run baselines recorded for **all discovered integration projects (8, repo state 2026-11-18; spec requested 6)** via CI evidence; local runs **not executed** due to environment blockers (QEMU/idf.py PATH/activated env validation). Build results/timings available from CI; local build baselines not run (documented).
- [x] `make lint` baseline attempted locally (full `make lint`, 74.31s, exit 2, failed with clang-tidy errors/warnings in this environment; compile DB present) and CI lint/build execution timings recorded.
- [x] CI runner timing comparison recorded from latest representative/successful `main` runs (consolidated CI with execution + latest successful standalone workflow runs requested by spec, including path-filter-skipped heavy-job cases documented).
- [ ] Cross-platform baselines (Linux/macOS/devcontainer) — **out of scope/unvalidated** in this Windows-only pass.
- [ ] Peak RAM/disk/log-volume metrics — **out of scope/unvalidated** in this pass.

## Notes

- The initial DX1 pass did not modify firmware/frontend source logic; it executed existing checks/scripts to gather timing baselines. The follow-up added local firmware build + compiler-cache measurements and documentation updates only (no firmware/frontend behavior changes).
- 2026-11-18 DX1 completion pass updated this report (docs/evidence only) and executed/recorded local checks: `make format-check` (pass, exit 0), `make docs-check` (pass, exit 0), `make test` (pass, exit 0, 30/30 CTest + 3 Python unittest tests, ~1.74s), and full `make lint` (failed, exit 2, ~74.31s) in the local Windows environment.
- A stale `build/` CMake cache from a different host path (`/workspaces/cos`) blocked local builds until `build/` was removed and recreated (2026-11-11).
- PowerShell/native-command stderr wrapping and ESP-IDF environment warnings were observed in some captured outputs but did not affect exit codes for the passing commands above (where applicable).
- Hardware-dependent validation was not performed.
- Spec note: DX1 slice requested integration baselines for "all 6" projects; current repository contains 8 integration test projects and this report records evidence for all 8 (and notes standalone workflow runs where heavy jobs were path-filter-skipped).

## Addendum (2026-11-20, DX3a CI/repo compiler caching integration)

**Status:** DX3a implementation/evidence follow-up recorded (docs + plan/queue updates; scoped workflow integration already present on `main`). This addendum does not change historical 2026-11-11/2026-11-18 measurements above.

- **Scoped CI workflows:** `.github/workflows/build-and-lint.yml` and `.github/workflows/integration-test.yml` enable `ccache`/`IDF_CCACHE_ENABLE=1` for ESP-IDF firmware builds in `espressif/idf:v6.1`, install/configure `ccache`, restore/save `.ccache` via `actions/cache` with runner/OS + `idf-v6.1` + build-input-hash keys and `restore-keys` fallbacks, and run `ccache --zero-stats` / `ccache --show-stats` around firmware builds. Existing `integration_test/test_*/build` directory caching remains in place for integration suites.
- **Observed CI evidence (main, required checks green, cache integration in effect):**
  - CI run [44559762592](https://github.com/gallayl/cos/actions/runs/44559762592) (`fix(esp_littlefs): harden vendored wrapper for stability and diagnostics`, head `4ef50c0470c561803003a6f13cd87cf6cf2f4540`, 2026-11-19): `lint / ESP-IDF Build & Lint` and all 8 `integration-tests / Integration Tests (...)` matrix jobs executed successfully with `Restore ccache` / `Post Restore ccache` steps (and build steps showing ccache enabled/stats). Build-and-lint log evidence showed cache restored by restore-key, `ccache --zero-stats`/`--show-stats` executed, and cache saved (sampled stats window reported 0% hits / 1334 misses for that build; hit rates vary by cache key/history).
  - CI run [44691115573](https://github.com/gallayl/cos/actions/runs/44691115573) (`feat(http): initial R24 user HTTP server API scaffold`, head `fb458adea5e97f3398ae11a15abc2eb5705a2ad5`, 2026-11-20): scoped build-and-lint and integration-test jobs executed successfully with ccache install/restore/build/stats/save steps; sampled build-and-lint log evidence also showed cache restored by restore-key and ccache stats/enablement reported.
- **Cache behavior/timing notes:** cache restore/save and ccache stats steps are visible in logs for runs where scoped jobs execute. Exact hit rates/timing deltas are workload-, key-, and cache-history-dependent and were not re-benchmarked as a controlled before/after in this DX3a delivery pass (DX1 local ~1.94× warm clean-build comparison remains the baseline timing evidence; CI timing deltas **not measured** in this pass).
- **DX3a PR/validation:** see `docs/plan/done/dx3-build-and-lint-performance.md` for delivery evidence, local checks, and PR CI run/link once recorded.
