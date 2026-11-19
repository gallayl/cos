# Harden vendored esp_littlefs for stability and diagnostics

- **ID:** esp-littlefs-stability-hardening
- **Status:** ready (approved; pre-approved for implementation/PR flow, merge remains human)
- **Created:** 2026-11-19
- **Source/links:** user request "create a new feature of this fixes" following audit of `components/esp_littlefs/` (2026-11-19, focus: stability, possible deadlocks, possible hangs, clear error reporting and hardware issue handling). Related: [`../2026-11-13-vendor-esp-littlefs-component.md`](../2026-11-13-vendor-esp-littlefs-component.md), [`../needs-clarification/vendor-esp-littlefs-component-validation.md`](../needs-clarification/vendor-esp-littlefs-component-validation.md), [`../../progress/2026-11-13-vendor-esp-littlefs.md`](../../progress/2026-11-13-vendor-esp-littlefs.md), [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md).
- **Approval:** User explicitly approved in chat on 2026-11-19 ("Move to \"Ready\"") after specification (decisions 1A–7A recorded). Pre-approved for implementation/PR flow; merge remains a human action.

## Summary
Harden the vendored `components/esp_littlefs/` package (esp_littlefs v1.23.1 integration used by COS filesystem support) by fixing audit-identified stability, race/deadlock, hang-risk, and diagnostics/error-handling issues while preserving required LittleFS mount/format/image-generation behavior for ESP32/ESP-IDF 6.1.

## Scope
- **In scope (all agreed fixes, applied across partition, SDMMC, BDL, and mmap code paths):**
  - **Hot-path logging:** downgrade/remove per-operation start/done `ESP_LOGI` traces in `src/littlefs_esp_part.c` and hot-path VFS traces in `src/esp_littlefs.c` (e.g. read/write/erase, open/mkdir start/done) to `ESP_LOGV` or remove them; keep `ESP_LOGE/W` and actionable hardware/error diagnostics.
  - **Format safety:** make `format_from_efs` / `esp_littlefs_format*` safe with respect to mounted/active use. Detect mounted/active contexts that cannot be safely exclusively formatted and reject unsafe format requests with a clear error (e.g. `ESP_ERR_INVALID_STATE`) and log context; do not unmount/format/remount while other operations may run. Callers must unmount/stop use before formatting.
  - **Registry synchronization:** synchronize `_efs[]` registry lookups/mutations consistently with `_efs_lock` (and appropriate lock ordering) in register/unregister/query paths to prevent races/use-after-free.
  - **Hang-risk handling (`portMAX_DELAY`):** keep existing `portMAX_DELAY` mutex-wait behavior (no timeout semantic change in this item) and document hang/deadlock limitations; reduce causes via locking/format-safety fixes. Bounded waits/timeout errors are explicitly deferred as follow-up work if needed.
  - **`F_GETPATH` truncation:** detect `snprintf` truncation/failure and return `-1` with `errno = ENAMETOOLONG` (or appropriate error contract) instead of reporting success for a cut-off path.
  - **Mmap cleanup:** guard `esp_partition_munmap` so it is called only when mmap succeeded/the handle is valid on init/cleanup paths.
  - **Bounds/error checks:** add explicit bounds/geometry/error checks and clearer error propagation/log context in partition/BDL/SDMMC backend operations where practical for hardware/config faults.
  - **Runtime asserts:** replace `assert(...)` in operational/runtime-reachable paths (e.g. caller/input validation, VFS/fcntl arguments, geometry/cache/FD state checks) with recoverable error returns/logs where appropriate; avoid changing true compile-time/invariant behavior unless clearly justified.
- **Validation target:** require evidence for COS default flash-partition LittleFS flow; validate SDMMC/BDL/mmap-specific paths when environment/config is available and document gaps.

## Non-goals
- Redesigning `portMAX_DELAY` waits into bounded timeouts or changing mutex timeout/error contracts in this item (documented/deferred).
- Replacing LittleFS or redesigning COS filesystem APIs beyond fixes needed here.
- Upstreaming changes to joltwallet/esp_littlefs unless separately decided.
- Broad refactors or style-only churn in vendored upstream core (`src/littlefs/`) beyond necessary, justified fixes.
- Hardware features/behaviors outside esp_littlefs/filesystem scope.

## Acceptance criteria
- [ ] All in-scope fixes above are implemented in partition, SDMMC, BDL, and mmap code paths as applicable, with agreed behavior changes: unsafe format requests on mounted/active contexts rejected with clear error/log (no concurrent unmount/format/remount), `F_GETPATH` truncation returns error (`-1`, `errno=ENAMETOOLONG`), mmap unmap guarded on failed init, and hot-path I/O start/done log noise removed/downgraded while actionable error diagnostics remain.
- [ ] Registry access and format/use paths are synchronized to address identified race/use-after-free and concurrent-format risks; `portMAX_DELAY` limitation is documented (no timeout semantic change).
- [ ] Runtime-reachable operational asserts/checks in scope are converted to recoverable error handling where appropriate, and backend bounds/error reporting is strengthened where practical.
- [ ] Automated validation required by `AGENTS.md` for affected scope passes and is recorded with evidence: `make format-check` and `make test` required; `make build` and `make lint` run in ESP-IDF v6.1 environment when available or concrete blockers recorded. `integration_test/test_esp_littlefs` (QEMU/target as applicable) run when environment is available; unavailable/hardware-only gaps (SD/MMAP/BDL error injection, known shell_fs/QEMU constraints) explicitly documented.
- [ ] Relevant docs/progress/plan tracking updated (including cross-links with `vendor-esp_littlefs-component-validation`) with evidence, behavior notes, and known limitations/follow-ups.

## Validation plan
- **Automated (required):** `make format-check`, `make test`.
- **Automated (when environment available; otherwise record concrete blocker):** `make build`, `make lint` in ESP-IDF v6.1 environment for firmware/C-C++ changes. Run `integration_test/test_esp_littlefs` (QEMU/target as applicable) when environment/tooling is available.
- **Hardware/manual:** none required by default. Only if explicitly needed and feasible for gaps not covered by host/QEMU/integration tests (e.g. real flash/SD/MMAP/BDL error injection or recovery behavior); record environment, steps, outcomes, and any known shell_fs/QEMU constraints.

## Expected areas/files
- `components/esp_littlefs/src/esp_littlefs.c`
- `components/esp_littlefs/src/littlefs_esp_part.c`
- `components/esp_littlefs/src/littlefs_bdl.c`
- `components/esp_littlefs/src/littlefs_sdmmc.c`
- `components/esp_littlefs/src/littlefs_api.h` / `components/esp_littlefs/include/esp_littlefs.h` (only if needed for clarified behavior/contracts; avoid unnecessary public API changes)
- `components/esp_littlefs/README.md` and/or maintenance notes (behavior/limitations/logging/format semantics as applicable)
- `integration_test/test_esp_littlefs/` and related integration coverage (if extended/executed)
- `docs/plan/needs-clarification/vendor-esp-littlefs-component-validation.md`, `docs/progress/2026-11-13-vendor-esp-littlefs.md`, `docs/plan/consolidated-improvement-plan.md` (cross-links/evidence as appropriate)

## Dependencies / prerequisites
- ESP-IDF v6.1 environment for build/lint/integration validation where available.
- Access to QEMU/integration-test environment and/or hardware as needed for filesystem behavior validation (optional paths/gaps documented if unavailable).
- Awareness of `docs/plan/needs-clarification/vendor-esp-littlefs-component-validation.md` and known shell_fs/QEMU hang investigation constraints (separate, cross-linked item).

## Risks / constraints
- `components/esp_littlefs` is vendored upstream-derived code; fixes diverge from upstream and increase maintenance burden.
- Concurrency/lifecycle changes (format rejection, locking) alter semantics for unsafe callers but are intended to prevent corruption/races.
- `portMAX_DELAY` waits remain; unrecoverable hangs are still possible if a driver/lock holder stalls (documented limitation).
- Some hardware faults (flash wear/errors, SD removal, driver stalls, BDL-specific failures) may be hard to reproduce deterministically in CI/QEMU.
- ESP32 is resource-constrained; fixes must avoid unnecessary RAM/CPU/log overhead and preserve `-Werror`/format/lint expectations where applicable.

## Open questions
- None.

## Decisions (2026-11-19)
- **Fix scope (A):** implement all 8 audit fixes — hot-path log noise reduction/removal, format-use safety, registry synchronization, `portMAX_DELAY` hang-risk handling per strategy below, `F_GETPATH` truncation error, mmap cleanup guard, bounds/error checks, and runtime-assert hardening in operational paths where appropriate.
- **Backends (A):** apply fixes across partition, SDMMC, BDL, and mmap code paths; require validation for COS default flash-partition flow and run other backend validation when environment/config is available (document gaps).
- **Format safety (A):** reject unsafe format requests when mounted/active with clear error (e.g. `ESP_ERR_INVALID_STATE`) and log context; no concurrent unmount/format/remount.
- **Hang waits (A):** keep `portMAX_DELAY` behavior (no timeout semantic change now), document hang/deadlock limitation; bounded waits/timeout errors deferred as follow-up if needed.
- **I/O logging (A):** downgrade/remove hot-path start/done `ESP_LOGI` traces to `ESP_LOGV` or remove; keep `ESP_LOGE/W` and actionable hardware/error diagnostics.
- **Validation coordination (A):** keep separate from `vendor-esp-littlefs-component-validation`, cross-linked; each records its own evidence (hardening evidence may support validation).
- **Validation / DoD (A):** `make format-check` and `make test` required; `make build` and `make lint` in ESP-IDF v6.1 when available or concrete blocker recorded; run `integration_test/test_esp_littlefs` when environment available; document unavailable/hardware-only gaps. No hardware/manual required unless explicitly needed and feasible.

## Progress / evidence
- 2026-11-19: created from audit of `components/esp_littlefs/` (stability/deadlock/hang/error-reporting/hardware-issue focus). No implementation started.
- 2026-11-19: clarification started (`clarify-feature`): identified scope/contract/validation gaps; `question` tool failed with argument-validation errors in this environment, so questions asked in plain chat. User agreed with all recommended answers (1A–7A). Spec/decisions recorded.
- 2026-11-19: user explicitly approved in chat ("Move to \"Ready\"") after specification. Status set to ready (pre-approved for implementation/PR flow; merge remains human), open questions none, and item moved to `docs/plan/ready/esp-littlefs-stability-hardening.md`. No implementation started.
- 2026-11-19 (implementation, `feat/esp-littlefs-stability-hardening`): implemented in-scope hardening across partition/SDMMC/BDL/mmap paths in vendored `components/esp_littlefs/`:
  - downgraded hot-path I/O start/done traces to `ESP_LOGV` in `src/littlefs_esp_part.c` (read/write/erase) and `src/esp_littlefs.c` (open/mkdir start/done) while keeping `ESP_LOGE/W` and actionable error diagnostics (e.g. SD card format completion remains `LOGI`).
  - `format_from_efs` / `esp_littlefs_format*` now reject unsafe format requests when mounted/active with `ESP_ERR_INVALID_STATE` and log context (no in-place unmount/format/remount); callers must unmount before formatting. Init mount-failure recovery remains supported for unmounted registered contexts.
  - synchronized registry lookups/mutations with a recursive global EFS lock; unregister paths detach registry entries under lock before VFS unregister/free; mounted/info/format helpers take registry state under lock.
  - `F_GETPATH` now validates buffer, detects `snprintf` truncation/failure, and returns `-1` with `errno = ENAMETOOLONG` (no success on truncated path); removed runtime `assert(buffer)` in that path.
  - guarded `esp_partition_munmap` to initialized mmap mappings only (`mmap_data != NULL`) in `esp_littlefs_free`.
  - added bounds/geometry/context/error checks and richer error logging (`esp_err_to_name`) in partition, BDL, and SDMMC backend operations where practical.
  - converted selected runtime-reachable asserts/checks to recoverable handling where appropriate (e.g. `esp_vfs_littlefs_register` arg validation, VFS path/ctx checks in open/stat/unlink/rename/opendir/mkdir/rmdir, format temp-context cleanup); `portMAX_DELAY` mutex waits kept (no timeout semantic change) and limitation documented in `components/esp_littlefs/README.md`.
  - updated `components/esp_littlefs/README.md` (COS hardening/behavior/limitations, corrected integration test paths).
  - updated integration tests to match safe format contract: `integration_test/test_esp_littlefs_{core,extra}/main/test_littlefs.c` ("can format mounted partition" → "format rejects mounted partition", expects `ESP_ERR_INVALID_STATE` while mounted and verifies format-after-unmount), and `.../main/test_benchmark.c` formats before mount in `setup_littlefs()`.
- **Validation (executed):**
  - `make format` — pass (exit 0). Reformatted sources including vendored `components/esp_littlefs` and touched integration tests.
  - `make format-check` — pass (exit 0).
  - `make test` — pass (exit 0). `ctest --test-dir test/build --output-on-failure`: 30/30 tests passed. `python -m unittest discover -s scripts -p "test_*.py"`: 3 tests ran, OK.
  - `make build` — pass (exit 0, ESP-IDF v6.1, `IDF_PATH=C:/esp/v6.1/esp-idf`, tools `C:/Espressif/tools`): configured/built successfully, generated `build/cos.bin` ("Project build complete"). Pre-existing warnings observed (Kconfig deprecation/value notes, `managed_components/joltwallet__littlefs` unexpected-dir warning, `ESP_ROM_ELF_DIR` gdbinit warning, unused `resolve_arg` in `components/shell/src/shell_fs_cmds.c`) — not introduced by this change. Initial build attempt failed due to declaration/order issues introduced during edit (`esp_littlefs_errno` visibility, `esp_littlefs_give_efs_lock` declaration); fixed and rebuild passed.
  - `make lint` — **not cleanly completed (environment/tooling blocker, pre-existing)**. Ran in ESP-IDF 6.1 environment but clang-tidy failed (exit 1) with many `clang-diagnostic-error` "file not found" (e.g. `light_sensor.h`, `system.h`, `boot_profile.h`, `service_diagnostics.h`, `service_lifecycle.h`, `bluetooth.h`) and widespread pre-existing warnings-as-errors across unrelated components; output also showed `found compiler error(s)` early in run. Vendored `components/esp_littlefs/**` is excluded from `make lint` SOURCES by project Makefile, so this run did not lint the modified esp_littlefs sources but did not produce a passing project lint result. Repro: `make lint` (requires properly generated/usable clang compile DB and clang-tidy environment for full project sources).
  - `make integration-test-build-test_esp_littlefs_core` — **build completed** (artifacts generated: `integration_test/test_esp_littlefs_core/build/test_esp_littlefs_core.bin`, bootloader/partition table; ninja finished 1048/1048, "Project build complete"). Background shell `sh_1f361847d0013MDMwPFgF1b81z` reported exit code 1 despite successful build output (reported factually). `python scripts/integration_test.py build test_esp_littlefs_core test_esp_littlefs_extra` via system Python failed immediately: `No module named 'rich_click'` (IDF Python env not used by that invocation).
  - `make integration-test-build-test_esp_littlefs_extra` — **build completed** (artifacts generated: `integration_test/test_esp_littlefs_extra/build/test_esp_littlefs_extra.bin`, bootloader/partition table; ninja 1051/1051, "Project build complete"). Background shell `sh_1f364ff5f001N3k69zR4xl0lTk` reported exit code 1 despite successful build output (reported factually).
  - `integration_test/test_esp_littlefs_{core,extra}` run (QEMU/target) — **not executed**: QEMU (`qemu-system-xtensa`) not found in this environment. Documented gap (requires ESP-IDF integration-test env + QEMU/target as applicable).
- **Not run / gaps:** `make lint` clean pass blocked by environment/tooling as above; integration test execution on QEMU/target not run (no QEMU). SDMMC/BDL/mmap-specific runtime validation and hardware fault/error injection not executed in this environment (code paths updated; requires target/hardware/config). No frontend/docs-check changes required by this scope. Related validation tracking remains in [`../needs-clarification/vendor-esp-littlefs-component-validation.md`](../needs-clarification/vendor-esp-littlefs-component-validation.md).
- 2026-11-19 (review/fix follow-up, `feat/esp-littlefs-stability-hardening`): independent `reviewer` subagent (`ses_e0c98946bffeEc1E4V0BFC10gN`, `code-review`, read-only) completed. Findings: **Must fix: none**. **Should fix (addressed):**
  1. `esp_littlefs_init` no longer holds the global registry lock across mount/format/grow I/O: slot/uniqueness checks and context publication happen under `_efs_lock`, then global lock is released for long-running `lfs_mount`/format-recovery/grow operations; per-FS lock is held during FS I/O and mount-fail recovery uses `format_from_efs(efs)` directly on the registered-but-unmounted context.
  2. Unregister paths (`partition_label`/SDMMC/partition/blockdev) no longer detach from `_efs[]` or free context on `esp_vfs_unregister` failure; on success they detach under global lock and free. Error logs include `esp_err_to_name`.
  3. `*_info` paths no longer hold the global registry lock across `lfs_fs_size`/usage calculation; `get_total_and_used_bytes` now validates mounted/usable state under per-FS lock and returns `ESP_ERR_INVALID_STATE` for unmounted contexts (prevents unsafe usage queries during init/for unmounted registered slots).
  - **Nice to have (addressed):** added `F_GETPATH` truncation regression assertions (`-1`, `errno == ENAMETOOLONG` with too-small buffer) in `integration_test/test_esp_littlefs_core/main/test_littlefs.c` and `integration_test/test_esp_littlefs_extra/main/test_littlefs.c` (guarded by `CONFIG_LITTLEFS_FCNTL_GET_PATH`; compile-validated, not runtime-executed here).
- **Post-fix validation (executed):**
  - `make format` — pass (exit 0).
  - `make format-check` — pass (exit 0).
  - `make test` — pass (exit 0). `ctest --test-dir test/build --output-on-failure`: 30/30 tests passed. `python -m unittest discover -s scripts -p "test_*.py"`: 3 tests, OK.
  - `make build` — pass (exit 0, ESP-IDF v6.1): `build/cos.bin` generated ("Project build complete"). Pre-existing warnings unchanged (Kconfig/managed_components/`ESP_ROM_ELF_DIR` gdbinit notes; unused `resolve_arg` in `components/shell/src/shell_fs_cmds.c`).
  - `make integration-test-build-test_esp_littlefs_core` — pass/build completed with latest fixes (`integration_test/test_esp_littlefs_core/build/test_esp_littlefs_core.bin`, "Project build complete"; shell capture reported exit code 1 despite successful output).
  - `make integration-test-build-test_esp_littlefs_extra` — pass/build completed with latest fixes (`integration_test/test_esp_littlefs_extra/build/test_esp_littlefs_extra.bin`, "Project build complete"; shell capture reported exit code 1 despite successful output).
  - QEMU/runtime execution and SDMMC/BDL/mmap/hardware fault-injection validation remain **not executed** in this environment.
- **Review status:** reviewer findings addressed in working tree and revalidated; no additional reviewer pass executed after fixes yet. Residual risks/gaps remain as above (concurrency/lifetime model is improved but not refcounted; runtime validation on target/QEMU/backends pending).
