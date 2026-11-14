# Unblock shell/FS integration pipeline (refactor + unit coverage)

**Created:** 2026-11-14  
**Status:** In progress (Phase 0 started)  
**Branch/PR:** `feat/r14-ts-diags` / [#22](https://github.com/gallayl/cos/pull/22)  
**Scope:** Stabilize and unblock `Integration Tests (shell_fs/init_sequence/system)` on CI by reverting speculative runtime guesses, then front-loading deterministic shell/FS behavior into host unit tests and adding minimal seams only where evidence requires it.  
**Related:**
- [`docs/plan/2026-11-14-unit-test-coverage-and-integration-rebalance.md`](2026-11-14-unit-test-coverage-and-integration-rebalance.md) — detailed inventory for R25 #11 (P1 VFS/shell-fs semantics, integration→unit rebalancing)
- [`docs/ideas/2026-11-10-refactoring-and-testability.md`](../ideas/2026-11-10-refactoring-and-testability.md) — R25 #4 (testability seams, console transport/task/driver), #11 (coverage enablement), #2 (startup composition)
- `AGENTS.md` — validation/reporting expectations

## Problem / evidence summary

- **Original `shell_fs` failures** (run [43588554529](https://github.com/gallayl/cos/actions/runs/43588554529)): runtime stalls in vendored LittleFS/FS command-time open/write paths (not test encoding); `28 passed / 3 failed` (`test_shell_alias_lifecycle`, `test_cp_copies_without_overwriting`, `test_rm_file`).
- **Subsequent churn (2026-11-14):** a series of speculative `components/shell/src/shell_input.c` UART/VFS/driver install-probe/reattach/lazy-install/backoff changes regressed boot/init in QEMU (`init_sequence`/`system` timeouts, UART driver errors/log floods, init hangs at various attach/probe points). These attempts did not establish a confirmed fix for the original `shell_fs` stalls.
- **Constraint:** no local ESP-IDF v6.1/QEMU environment; `make build`/`make lint`/integration validation depends on CI. Host checks available locally: `make format-check`, `make test`.

## Goal

Unblock the PR pipeline by **refactoring problematic areas and increasing host unit-test coverage** so deterministic failures are caught early, while keeping integration tests for true platform/runtime behavior (REPL/console transport, mounted LittleFS, command registration/startup, Berry-on-target, async console).

## Non-goals

- Further trial-and-error UART/console init tweaks without evidence and tests.
- Weakening integration assertions except where explicitly rebalanced to an equivalent host test.
- Broad R25 work (startup decomposition, unrelated Berry/UI/network refactors, vendored `esp_littlefs` rewrite) unless required by evidence from this pipeline.

## Baseline / revert

- **Baseline:** `273115c` (`fix(shell): restore UART VFS attach for console-owned driver`) for shell input + related host mocks — pre-churn state before driver install/probe/backoff guessing. Known: `273115c` alone was not proven green for later stalls; used here as a clean baseline, not a claimed fix.
- **Revert commit (local, created before this plan doc):** `ab11e98` — `revert(shell): restore shell input and UART mocks to 273115c baseline`
  - Restores: `components/shell/src/shell_input.c`, `test/mocks/driver/uart.h`, `test/mocks/mock_uart.c`, `test/mocks/freertos/task.h`, `test/mocks/mock_freertos_extra.c`
- **Legitimate test fixes retained in history:** `e8a70e1`, `7a69991`, `cf4d242` (`integration_test/test_shell_fs/test_shell_fs.py`).

## Progress tracking

| Phase | Commit(s) | Status | Notes |
|---|---|---|---|
| Plan doc + tracking | `37075c2` | Done | Persist plan, cross-refs, baseline |
| 0 — Baseline gate | `ab11e98`, `37075c2` (pushed) | Done (evidence collected) | Baseline [43613402861](https://github.com/gallayl/cos/actions/runs/43613402861) and Phase 1 [43613609943](https://github.com/gallayl/cos/actions/runs/43613609943): all non-`shell_fs` integration jobs passed; `shell_fs` failed in both. Classification: **A (shell input init hang, intermittent)** blocks `shell_fs` prompt readiness on reverted `273115c` (`uart_vfs_dev_use_driver` path); original 43588554529 **B** FS command-time stalls not re-observed because REPL never became interactive. |
| 1 — Shell FS command unit tests (quick win) | `5c52857` | Done (host) | Strengthened `mock_filesystem` stream/copy behavior and added host `shell_fs_cmds` tests (`cat` read/missing/dir, `rm` missing, `touch` preserves content, `cp` preserves content). `make format-check`/`make test` pass locally. |
| 2 — VFS contract tests / minimal seam + targeted fix | `faa0ecb`, `27d7e69`, (this commit) | In progress | Added `vfs_path` normalization contract test. Narrow B mitigations: `vfs_list_dir` skips dir `stat` when unsynced; `cmd_ls` prefers direct `opendir/readdir` (avoids per-entry stat/sizes) with fallback to `vfs_list_dir` for mocked/compat paths. CI validation pending for remaining non-`ls` stalls (open/mkdir/copy/read hangs). |
| 3 — Console/input seam (only if needed) | `3d5f4ed`, `f8192fd`, `abf7e89`, `667704c` | Done (evidence-backed) | `667704c` switched RX to `stdin`/`fread` (no UART driver install/attach APIs). [43622457867](https://github.com/gallayl/cos/actions/runs/43622457867): `system` and `init_sequence` passed; `shell_fs` reached interactive REPL (`Shell input ready`, prompt, commands delivered). Remaining `shell_fs` failures are runtime FS/VFS/LittleFS stalls (B), no longer input/init hangs. |
| 4 — R25 #4 console transport/input seam (follow-up) | (this commit) | Done (host) | Added injectable console read transport seam in `components/shell/src/shell_input.{h,c}` (`shell_input_set_transport`, default stdin reader preserved) per [R25 #4](../ideas/2026-11-10-refactoring-and-testability.md) / [coverage plan](2026-11-14-unit-test-coverage-and-integration-rebalance.md) to decouple RX policy from transport and enable host-testable input paths; `make format-check`/`make test` pass locally. Integration behavior unchanged by default. |
| Pipeline green | — | Pending | Required PR checks green; root causes evidence-backed |

## Phase details

### Phase 0 — Baseline gate
1. Push `ab11e98` and this plan doc; observe CI once to re-establish clean failure signatures on reverted baseline.
2. Download failed logs/artifacts and classify:
   - **A:** shell input/console transport boot (init hangs, UART driver/read errors, missing readiness/prompt)
   - **B:** VFS/flash/LittleFS command-path runtime (mounted FS stalls/errors)
   - **C:** deterministic command/arg/output/path logic suitable for host tests
3. Record findings in this document and choose next evidence-backed step (prioritize C via Phase 1, B contract/runtime split via Phase 2, A via Phase 3 only if blocking).

**Validation:** CI observation + artifact review (no local QEMU). Report run/job IDs and signatures.

### Phase 1 — Shell FS command semantics (host unit tests)
*Aligns with [coverage/rebalance plan](2026-11-14-unit-test-coverage-and-integration-rebalance.md) P1 “Shell FS command semantics” and quick wins.*

- Add/extend host tests invoking `components/shell/src/shell_fs_cmds.c` command handlers against `test/mocks/mock_filesystem` (and `vfs_path`/console mocks as needed) for success/error paths: `ls`, `cat`, `touch`, `mkdir`, `cd`, `pwd`, `rm`, `rmdir`, `cp`, `mv`, `hexdump`, `info` (as applicable), including nonexistent targets, dir-not-empty, overwrite/copy rules, relative resolution.
- Prioritize deterministic assertions related to original `shell_fs` failures: `rm` file behavior, `cp` copies without overwriting, and command-layer aspects of alias lifecycle where applicable.
- **Rebalance:** when a behavior is covered by host tests, keep integration focused on REPL/prompt/editing, UART/console transport, mounted LittleFS, registration/startup, Berry-on-target, async console. Update `integration_test/test_shell_fs` only if/when assertions are explicitly moved with equivalent confidence (track here).

**Validation:** `make format-check`, `make test` (host). CI for integration impact.

### Phase 2 — VFS contract tests / minimal seam + targeted fix
*Aligns with P1 “VFS high-level API behavior”.*

- Add host contract tests for `components/filesystem/src/vfs.c` (and coordination with `flash.c`/path rules): arg handling, path normalization, error mapping, file/dir semantics, overwrite/list/exists. Introduce the smallest seam/mocked backend needed for contracts not covered by existing mocks.
- If Phase 0/CI shows contract/logic defects, fix in VFS/shell-fs layer with unit coverage. If failures are vendored LittleFS/mount/runtime-specific, make narrow evidence-backed changes and validate in integration; document limits.

**Validation:** `make format-check`, `make test`; CI integration.

### Phase 3 — Console/input seam (only if Phase 0 shows A blocking)
*Aligns with R25 #4 (console transport/task/driver seams), narrowly scoped.*

- Evidence-backed fix delivered (`667704c`, stdin RX) to unblock A without UART driver/VFS attach churn.
- **R25 #4 follow-up (implemented):** added minimal injectable console read transport seam in `shell_input` (`shell_input_set_transport`, default stdin reader unchanged) so transport/read policy is decoupled from the RX task and host-testable. Further init/task seams remain future work if needed.
- `init_sequence`/`system` remain E2E; no speculative install/probe/VFS-reattach loops in init.

**Validation:** `make format-check`, `make test`; CI integration.

## Execution rules

- **One commit per phase** (plan doc/setup may be its own commit; revert already committed as `ab11e98`).
- Run relevant host checks proactively after changes (`make format-check`, `make test`); fix and re-run until pass. Report exact commands/outcomes.
- Do not claim build/lint/integration pass without environment/CI evidence; explain blockers and give actionable rerun steps per `AGENTS.md`.
- Update this document’s progress table/notes as phases complete or when CI evidence changes direction.

## CI observations

- **Phase 2 attempt (27d7e69)** — `vfs_list_dir`: avoid `stat` for `DT_DIR` entries when time is unsynced to reduce LittleFS operations in `ls`/walks.
- **Phase 2 CI (27d7e69)** — Integration run [43625023691](https://github.com/gallayl/cos/actions/runs/43625023691): **completed, failure**.
  - **Passed:** `init_sequence` [132538830869](https://github.com/gallayl/cos/actions/runs/43625023691/job/132538830869), `system` [132538830766](https://github.com/gallayl/cos/actions/runs/43625023691/job/132538830766), `vfs_ops` [132538830856](https://github.com/gallayl/cos/actions/runs/43625023691/job/132538830856), `nvs_calibration` [132538830846](https://github.com/gallayl/cos/actions/runs/43625023691/job/132538830846), `stability` [132538830832](https://github.com/gallayl/cos/actions/runs/43625023691/job/132538830832), `esp_littlefs` [132538830619](https://github.com/gallayl/cos/actions/runs/43625023691/job/132538830619).
  - **Failed:** `shell_fs` [132538830850](https://github.com/gallayl/cos/actions/runs/43625023691/job/132538830850) — **17 passed, 14 failed** (timeout 15m). Failures remain B-type mid-command stalls (`ls`/`touch`/`mkdir`/`cp`/`mv`/`rm`/`rmdir`/alias/Berry FS paths), with pass/fail set shifting vs 43622457867 (likely QEMU/LittleFS flakiness sensitivity). No clear improvement from dir-stat skip alone; continue targeted, evidence-backed investigation.

- **Phase 2 CI (a989fa3, cmd_ls fast path)** — Integration run [43626452654](https://github.com/gallayl/cos/actions/runs/43626452654): **completed, failure** (improved).
  - **Passed:** `init_sequence` [132543243411](https://github.com/gallayl/cos/actions/runs/43626452654/job/132543243411), `system` [132543243640](https://github.com/gallayl/cos/actions/runs/43626452654/job/132543243640), `vfs_ops` [132543243682](https://github.com/gallayl/cos/actions/runs/43626452654/job/132543243682), `nvs_calibration` [132543243642](https://github.com/gallayl/cos/actions/runs/43626452654/job/132543243642), `stability` [132543243652](https://github.com/gallayl/cos/actions/runs/43626452654/job/132543243652), `esp_littlefs` [132543243674](https://github.com/gallayl/cos/actions/runs/43626452654/job/132543243674).
  - **Failed:** `shell_fs` [132543243678](https://github.com/gallayl/cos/actions/runs/43626452654/job/132543243678) — **19 passed, 12 failed** (down from 17/14). `ls`/alias-dir listing stalls improved by direct readdir path; remaining stalls: `autocomplete_command_and_path`, `shell_alias_lifecycle`, `berry_filesystem_and_network_apis`, `berry_console_read_line_async`, `nested_mkdir`, `cp_copies_without_overwriting`, `cp_rejects_directory_copy`, `mv_moves_file`, `rmdir_only_removes_empty_directories`, `cd_relative`, `rm_file`, `alias_del` (all mid-command FS/console-op hangs).
- **Embedded log analysis (43626452654):** hangs occur inside LittleFS syscalls after successful seeding/REPL — e.g. `touch`/alias write hangs after `vfs_write_file: fopen` (no return), `mkdir` hangs after `vfs_mkdir` start (no `ok`), `cat`/Berry FS ops stall mid-command. `esp_littlefs` uses recursive mutex (no obvious open/mkdir deadlock in SPIFFS_COMPAT path). Added diagnostic `ESP_LOGI` around `lfs_file_opencfg` and `lfs_mkdir`.
- **Phase 2 CI (790df49, esp_littlefs instrumentation)** — Integration run [43632684277](https://github.com/gallayl/cos/actions/runs/43632684277): **completed, failure** (improved).
  - **Passed:** `init_sequence` [132562548968](https://github.com/gallayl/cos/actions/runs/43632684277/job/132562548968), `system` [132562548909](https://github.com/gallayl/cos/actions/runs/43632684277/job/132562548909), `vfs_ops` [132562548929](https://github.com/gallayl/cos/actions/runs/43632684277/job/132562548929), `nvs_calibration` [132562548739](https://github.com/gallayl/cos/actions/runs/43632684277/job/132562548739), `stability` [132562548962](https://github.com/gallayl/cos/actions/runs/43632684277/job/132562548962), `esp_littlefs` [132562548924](https://github.com/gallayl/cos/actions/runs/43632684277/job/132562548924).
  - **Failed:** `shell_fs` [132562548882](https://github.com/gallayl/cos/actions/runs/43632684277/job/132562548882) — **21 passed, 10 failed**. Remaining: `shell_alias_lifecycle`, `berry_filesystem_and_network_apis`, `nested_mkdir`, `cp_copies_without_overwriting`, `cp_rejects_directory_copy`, `mv_moves_file`, `rmdir_only_removes_empty_directories`, `cd_relative`, `rm_file`, `alias_del`.
- **Embedded log evidence (43632684277):** instrumented logs show stalls inside LittleFS core calls with no `done`: e.g. `mkdir /flash/outer` → `lfs_mkdir start: /outer` (no done), `touch`/alias persist → `lfs_file_opencfg start: /deleteme.txt` / `/.cos_aliases` (no done), `cp` dest open → `lfs_file_opencfg start: /copied.txt` (no done), Berry FS script write → `lfs_file_opencfg start: /berry-note.txt` (no done). Seeding ops all completed with `res=0`. Root cause is in vendored LittleFS core/block-device behavior under QEMU (`components/esp_littlefs/src/littlefs/lfs.c` and/or flash block layer), not shell/VFS glue.

- **Phase 0 baseline (revert `ab11e98` + plan `37075c2`)** — Integration run [43613402861](https://github.com/gallayl/cos/actions/runs/43613402861) (head `37075c2`): **completed, failure**.
  - **Passed:** `init_sequence` ([132503147764](https://github.com/gallayl/cos/actions/runs/43613402861/job/132503147764)), `system` ([132503147661](https://github.com/gallayl/cos/actions/runs/43613402861/job/132503147661)), `nvs_calibration` ([132503147747](https://github.com/gallayl/cos/actions/runs/43613402861/job/132503147747)), `vfs_ops` ([132503147666](https://github.com/gallayl/cos/actions/runs/43613402861/job/132503147666)), `stability` ([132503147649](https://github.com/gallayl/cos/actions/runs/43613402861/job/132503147649)), `esp_littlefs` ([132503147748](https://github.com/gallayl/cos/actions/runs/43613402861/job/132503147748)).
  - **Failed:** `shell_fs` ([132503147444](https://github.com/gallayl/cos/actions/runs/43613402861/job/132503147444)) — `1 passed, 30 failed` (timeout after 15m). `test_shell_boots` **PASSED**; all subsequent tests failed with ~30s `pexpect` timeouts waiting for prompt/command responses.
  - **Artifacts/log evidence:** in passing `test_shell_boots` log, boot reached `SHELL_READY` and prompt `COS/flash>` (autoexec OK). In failing tests (e.g. `test_pwd_default`, `test_rm_file`), boot often stalls after `shell_script: Script path ready` (or earlier command-registration logs) with **no** `shell_input: Shell input ready`, **no** prompt, **no** `SHELL_READY` → indicates hang inside `shell_input_init()` on reverted `273115c` path (`uart_vfs_dev_use_driver`/UART VFS attach), intermittently in `shell_fs` app context despite `init_sequence`/`system` passing.

- **Phase 1 push** — Integration run [43613609943](https://github.com/gallayl/cos/actions/runs/43613609943) (head `5c52857`): **completed, failure**.
  - **Passed:** `init_sequence` ([132503784365](https://github.com/gallayl/cos/actions/runs/43613609943/job/132503784365)), `system` ([132503784149](https://github.com/gallayl/cos/actions/runs/43613609943/job/132503784149)), `nvs_calibration` ([132503784127](https://github.com/gallayl/cos/actions/runs/43613609943/job/132503784127)), `vfs_ops` ([132503784177](https://github.com/gallayl/cos/actions/runs/43613609943/job/132503784177)), `stability` ([132503784106](https://github.com/gallayl/cos/actions/runs/43613609943/job/132503784106)), `esp_littlefs` ([132503783932](https://github.com/gallayl/cos/actions/runs/43613609943/job/132503783932)).
  - **Failed:** `shell_fs` ([132503784207](https://github.com/gallayl/cos/actions/runs/43613609943/job/132503784207)) — same signature as baseline (`1 passed, 30 failed`, timeout after 15m; `test_shell_boots` only pass).

- **Phase 3 attempt (3d5f4ed)** — Integration run [43617453485](https://github.com/gallayl/cos/actions/runs/43617453485): **completed, failure**.
  - **Passed:** `init_sequence` [132515521707](https://github.com/gallayl/cos/actions/runs/43617453485/job/132515521707), `system` [132515521525](https://github.com/gallayl/cos/actions/runs/43617453485/job/132515521525), `nvs_calibration` [132515521702](https://github.com/gallayl/cos/actions/runs/43617453485/job/132515521702), `vfs_ops` [132515521683](https://github.com/gallayl/cos/actions/runs/43617453485/job/132515521683), `stability` [132515521662](https://github.com/gallayl/cos/actions/runs/43617453485/job/132515521662), `esp_littlefs` [132515521688](https://github.com/gallayl/cos/actions/runs/43617453485/job/132515521688).
  - **Failed:** `shell_fs` [132515521732](https://github.com/gallayl/cos/actions/runs/43617453485/job/132515521732) — **1 passed, 30 failed**, timeout 15m. Stalls after `Script path ready` with no `Shell input ready`.

- **Phase 3 attempt (f8192fd)** — Integration run [43619402955](https://github.com/gallayl/cos/actions/runs/43619402955) (head `f8192fd`): **completed, failure**.
  - **Passed:** `init_sequence` [132521425270](https://github.com/gallayl/cos/actions/runs/43619402955/job/132521425270), `system` [132521425290](https://github.com/gallayl/cos/actions/runs/43619402955/job/132521425290), `nvs_calibration` [132521425375](https://github.com/gallayl/cos/actions/runs/43619402955/job/132521425375), `vfs_ops` [132521425255](https://github.com/gallayl/cos/actions/runs/43619402955/job/132521425255), `stability` [132521425389](https://github.com/gallayl/cos/actions/runs/43619402955/job/132521425389), `esp_littlefs` [132521425304](https://github.com/gallayl/cos/actions/runs/43619402955/job/132521425304).
  - **Failed:** `shell_fs` [132521425119](https://github.com/gallayl/cos/actions/runs/43619402955/job/132521425119) — **1 passed, 30 failed**, timeout 15m. `test_shell_boots` reached `Shell input ready`/`SHELL_READY`; failing cases stalled after `Script path ready` with no `Shell input ready` and only a single `uart_read_bytes` driver error (RX task hung during lazy install/attach attempt).

- **Phase 3 attempt (abf7e89)** — Integration run [43621326936](https://github.com/gallayl/cos/actions/runs/43621326936) (head `abf7e89`): **completed, failure**.
  - **Passed:** `init_sequence` [132527463117](https://github.com/gallayl/cos/actions/runs/43621326936/job/132527463117), `vfs_ops` [132527463036](https://github.com/gallayl/cos/actions/runs/43621326936/job/132527463036), `nvs_calibration` [132527463033](https://github.com/gallayl/cos/actions/runs/43621326936/job/132527463033), `stability` [132527462977](https://github.com/gallayl/cos/actions/runs/43621326936/job/132527462977), `esp_littlefs` [132527463024](https://github.com/gallayl/cos/actions/runs/43621326936/job/132527463024).
  - **Failed:** `shell_fs` [132527462965](https://github.com/gallayl/cos/actions/runs/43621326936/job/132527462965) — **1 passed, 30 failed**, timeout 15m. Reached `Shell input ready`/`SHELL_READY` but continuous `uart_read_bytes(1742): uart driver error` spam (no UART driver) flooded logs and broke prompt/command I/O. `system` [132527463272](https://github.com/gallayl/cos/actions/runs/43621326936/job/132527463272) — failed with same spam after `SYSTEM_READY`.

- **Phase 3 fix (667704c)** — Integration run [43622457867](https://github.com/gallayl/cos/actions/runs/43622457867) (head `667704c`): **completed, failure** (major progress on A).
  - **Passed:** `init_sequence` [132530934059](https://github.com/gallayl/cos/actions/runs/43622457867/job/132530934059), `system` [132530934021](https://github.com/gallayl/cos/actions/runs/43622457867/job/132530934021), `vfs_ops` [132530933781](https://github.com/gallayl/cos/actions/runs/43622457867/job/132530933781), `nvs_calibration` [132530933994](https://github.com/gallayl/cos/actions/runs/43622457867/job/132530933994), `stability` [132530934052](https://github.com/gallayl/cos/actions/runs/43622457867/job/132530934052), `esp_littlefs` [132530933985](https://github.com/gallayl/cos/actions/runs/43622457867/job/132530933985).
  - **Failed:** `shell_fs` [132530934010](https://github.com/gallayl/cos/actions/runs/43622457867/job/132530934010) — **18 passed, 13 failed**, timeout 15m. REPL is now interactive (prompt/`Shell input ready`, commands received), but 13 tests stall during FS operations consistent with original **B** (LittleFS/VFS command-time hangs), e.g. `touch`/file create hangs after fopen (no fwrite/fclose), `ls` hangs with no output, `mkdir`/`cp`/`mv`/`rm`/alias ops stall mid-command.
- **Conclusion:** A/input-transport issues addressed by `667704c` (`stdin`/`fread` RX, no UART driver API calls) — `init_sequence`/`system` green. **B** (runtime LittleFS/VFS stalls in `shell_fs` workload) is now the blocker and requires targeted, evidence-backed investigation/fixes in VFS/flash/esp_littlefs command paths (Phase 2), validated in CI (no local ESP-IDF v6.1/QEMU).

## Skipped / risks (living)

- **Blocker (B, vendored LittleFS/QEMU):** 10 `shell_fs` cases still fail ([43632684277](https://github.com/gallayl/cos/actions/runs/43632684277), **21/31 pass**). Embedded logs with instrumentation prove hangs inside `lfs_file_opencfg`/`lfs_mkdir` (core `lfs.c`/block-device path) with no return, after successful seeding/REPL. I cannot locally reproduce or step-debug without ESP-IDF v6.1+QEMU; CI logs are sufficient to localize but not to pinpoint the internal lfs/block-driver cause. Next evidence-backed options: instrument `lfs.c`/block read-prog-erase/sync paths via CI, compare `test_shell_fs` vs `vfs_ops`/`esp_littlefs` mount/config/usage patterns, or patch LittleFS/block layer once cause is identified.
- Over-rebalancing can drop needed platform coverage — keep REPL/console/mounted-FS/Berry-on-target paths in integration.
- Not run locally: `make build`, `make lint`, integration/QEMU (environment unavailable). Untracked local CI artifact dirs remain.
