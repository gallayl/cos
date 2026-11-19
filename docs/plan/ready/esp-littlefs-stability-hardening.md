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
