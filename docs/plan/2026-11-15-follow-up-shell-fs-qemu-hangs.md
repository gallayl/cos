# Follow-up: re-enable `shell_fs` integration tests (QEMU LittleFS flash-read hangs)

**Status:** Open — revalidation in progress (tests re-enabled in branch/PR for CI evidence; historical workaround context below)  
**Created:** 2026-11-15  
**Updated:** 2026-11-20  
**Branch/PR context:** `feat/r14-ts-diags` / PR #22 (historical); revalidation on `fix/shell-fs-qemu-hangs-revalidate` / PR [#108](https://github.com/gallayl/cos/pull/108)  
**Related:** [2026-11-14-unblock-shell-fs-pipeline.md](2026-11-14-unblock-shell-fs-pipeline.md), [2026-11-14-unit-test-coverage-and-integration-rebalance.md](2026-11-14-unit-test-coverage-and-integration-rebalance.md), [../ideas/2026-11-10-refactoring-and-testability.md](../ideas/2026-11-10-refactoring-and-testability.md), [ready/unblock-shell-fs-pipeline-and-qemu-hangs.md](ready/unblock-shell-fs-pipeline-and-qemu-hangs.md)

## Summary

To unblock PR #22 merging, a subset of `integration_test/test_shell_fs/test_shell_fs.py` cases known to hang under QEMU were temporarily disabled (skip/TODO markers). Other integration suites and non-`shell_fs` coverage remained green on CI at the time; host/unit coverage for shell FS/VFS behavior was expanded as part of the unblock effort.

**2026-11-20 revalidation:** as part of the ready item, the 11 historically listed tests and the combined `test_shell_fs` case were re-enabled (skip markers removed) to obtain current CI evidence under ESP-IDF v6.1 + bundled QEMU. Outcome/CI run IDs pending; update this section with resolution or remediation evidence when available.

## Current disabled tests

Disabled in `integration_test/test_shell_fs/test_shell_fs.py` (pending re-enable when root cause fixed/mitigated):

- `test_autocomplete_command_and_path`
- `test_shell_alias_lifecycle`
- `test_berry_filesystem_and_network_apis`
- `test_nested_mkdir`
- `test_cp_copies_without_overwriting`
- `test_cp_rejects_directory_copy`
- `test_mv_moves_file`
- `test_rmdir_only_removes_empty_directories`
- `test_cd_relative`
- `test_rm_file`
- `test_alias_del`

> Note: latest HEAD CI (run [43640354444](https://github.com/gallayl/cos/actions/runs/43640354444), Integration Tests, failure on `2c264cf`) could not be broken down to per-test failures during this change due to GitHub API connectivity issues; the disabled set reflects the latest available evidence-backed failing/hanging cases from [43633877205](https://github.com/gallayl/cos/actions/runs/43633877205) / [43634305900](https://github.com/gallayl/cos/actions/runs/43634305900) (20/31 pass in `shell_fs`, 11 failures).

## Evidence / root-cause hypothesis

- **Symptom:** affected cases hang mid-command (timeouts) while REPL is interactive and other suites (`init_sequence`, `system`, `vfs_ops`, `nvs_calibration`, `stability`, `esp_littlefs`) pass.
- **Instrumentation evidence (esp_littlefs):** repeated LittleFS metadata scans issue block reads where `part_read start` is logged but the matching `part_read done` never arrives (e.g. hangs observed at `blk=1 off=2304/3072 size=128` in failing cases), indicating `esp_partition_read(...)` does not return in QEMU flash emulation for those specific reads.
- **Code locations instrumented/mitigated:** `components/esp_littlefs/src/esp_littlefs.c`, `components/esp_littlefs/src/littlefs_esp_part.c`.
- **Mitigations tried:** `vfs_list_dir` dir-stat skip, `cmd_ls` direct readdir fast path, `vTaskDelay(1)` yield before flash reads (regressed; reverted) — see unblock plan for commits/runs.
- **Environment:** CI uses `espressif/idf:v6.1` with QEMU installed via IDF tools (`qemu-xtensa`, bundled `esp_develop_9.2.2_20260417`); local ESP-IDF v6.1 + QEMU environment was unavailable for interactive debugging.

## Re-enable criteria

- [ ] Identify/fix or reliably mitigate the QEMU `esp_partition_read` hang during LittleFS metadata operations triggered by `shell_fs` workload (evidence-backed), **or** document evidence that the issue is no longer reproduced in current CI.
- [~] Restore disabled tests in `integration_test/test_shell_fs/test_shell_fs.py` and remove TODO markers. **2026-11-20:** removed 12 skip/TODO markers (combined `test_shell_fs` + 11 historically listed tests) in revalidation branch/PR; pending CI confirmation.
- [ ] `shell_fs` integration suite passes consistently on CI (no command-time hangs/timeouts) alongside other integration suites.
- [ ] Update [2026-11-14-unblock-shell-fs-pipeline.md](2026-11-14-unblock-shell-fs-pipeline.md) with resolution evidence and mark pipeline green.

## 2026-11-20 revalidation log (CI-authoritative)

- Baseline: `origin/main@5d8b343` (main CI [44744734342](https://github.com/gallayl/cos/actions/runs/44744734342) success). Branch: `fix/shell-fs-qemu-hangs-revalidate`.
- Change: re-enabled all skipped `shell_fs` tests in `integration_test/test_shell_fs/test_shell_fs.py` (no local ESP-IDF v6.1/QEMU).
- PR: [#108](https://github.com/gallayl/cos/pull/108) (draft), head commits `cc12fe0`, `0d707e0` initially.
- CI run [44748487138](https://github.com/gallayl/cos/actions/runs/44748487138) (head `0d707e0`): **failure**. `shell_fs` job [136329951615](https://github.com/gallayl/cos/actions/runs/44748487138/job/136329951615) failed (4m40s, 12 failed / 13 warnings); all other required integration suites passed (`esp_littlefs_core`, `esp_littlefs_extra`, `init_sequence`, `nvs_calibration`, `stability`, `system`, `vfs_ops`). Evidence (`integration-test-logs-shell_fs` artifact 14634781111): `test_shell_fs` timed out waiting for `hello.txt` after `ls /flash` because embedded `/flash/ls.be` failed with `'string' value has no method 'charAt'` (`ls: ESP_FAIL`); 11 individual tests timed out waiting for prompt and their `dut.log` files showed boot stopped after filesystem mount/SD-card-disabled log before shell init/seeding completion (possible historical hang recurrence/flakiness; inconclusive). Mitigation applied in `4965e3d`: transpiler lowers `charAt` to Berry range and canonical `ls.be`/`dir.be` regenerated; per-test boot-marker waits added in `test_shell_fs.py`.
- CI run [44751146082](https://github.com/gallayl/cos/actions/runs/44751146082) (head `4965e3d`): **failure**. `shell_fs` job [136339179879](https://github.com/gallayl/cos/actions/runs/44751146082/job/136339179879) failed (8m43s, **5 failed, 7 passed, 13 warnings**); all other required integration suites passed. Evidence: `ls.be` stalled on `ls /flash` (shadowing/`slice`), alias lifecycle test rejected script-resolved target, autocomplete path case timed out, and `berry_api.be` stalled after `S1` (possible FS hang, inconclusive). Fix `8575459`: corrected `ls/dir` scripts and adjusted alias test.
- CI run [44753607575](https://github.com/gallayl/cos/actions/runs/44753607575) (head `8575459`): **failure**. `shell_fs` job [136347566017](https://github.com/gallayl/cos/actions/runs/44753607575/job/136347566017) failed (8m28s, **4 failed, 8 passed, 13 warnings**); all other required integration suites/checks passed. Evidence: alias lifecycle passes; `ls /flash` still stalls in `test_shell_fs`/`test_mv_moves_file` after compile, `test_autocomplete_command_and_path` still times out on path completion, `test_berry_filesystem_and_network_apis` still stalls after `S1` before `FS_LIST_OK`. Mitigation attempt pending push: simplify `ls/dir` scripts; remaining hangs may require local QEMU/ESP-IDF debugging (hard blocker risk).

## Suggested next investigations

- Compare `test_shell_fs` mount/config/operation patterns vs `vfs_ops`/`esp_littlefs` suites to isolate triggering conditions under QEMU.
- Try running with/without additional esp_littlefs debug or alternative flash/cache config knobs in the test app (narrow, evidence-motivated).
- Evaluate newer QEMU (`esp-develop-9.2.2-20261109` upstream) via explicit CI override if feasible; IDF-bundled QEMU for v6.1/v6.1.1 is currently `20260417`.
- If hang is confirmed upstream/emulation-specific, consider targeted workaround or scoping tests with documented rationale and compensating host coverage (only with explicit approval per AGENTS/rebalance rules).
