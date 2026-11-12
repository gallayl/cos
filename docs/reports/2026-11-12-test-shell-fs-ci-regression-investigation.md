# 2026-11-12: `test_shell_fs` CI regression investigation

## Summary

- **Question:** Is the fresh CI failure the same as the original `test_berry_filesystem_and_network_apis` failure?
- **Answer:** **No.** The original Berry FS/NET integration hang/timeout is no longer the failing case in the reported CI run(s). New failures have appeared across different CI runs, with a mix of known and newly observed symptoms.
- **Status:** Investigation in progress (updated 2026-11-12 for merge/run at head `944cf49`, PR #22). Root causes are partially identified; fixes are being applied, but local QEMU/LittleFS validation on this Windows host remains unreliable (command stalls), so CI re-validation is required.

## Latest CI evidence (provided job log, 2026-11-12)

Source: user-provided GitHub Actions job log blob (job `Integration Tests (QEMU)`, container `espressif/idf:v6.1`), checkout `944cf49` (`Merge 8c181d814e66e4c55d57701c0507540402d569cc into a9da155...`).

- **Result:** `4 failed, 27 passed, 31 warnings` in `test_shell_fs` (pytest `--embedded-services idf,qemu`).
- **Failures:**
  - `test_shell_alias_lifecycle` — `TIMEOUT` expecting `Unknown command: ll` after `unalias ll` / `ll`. Buffer: `unalias ll` (no "Unknown command" line observed).
  - `test_ping_reports_unavailable_network` — `TIMEOUT` expecting `ping: example.com failed (ESP_ERR_INVALID_STATE)`. Buffer showed `I (...) main_task: Returned from app_main()` followed by `ping example.com` and `Unknown command: ping example.com` (duplicate prompts). **New vs earlier runs in this report:** `ping` is now script-provided after commit `8c181d8` (`Ping script migration...`), and the integration test app did not seed `/flash/ping.be`, so script dispatch could not resolve `ping`.
  - `test_cp_copies_without_overwriting` — `TIMEOUT` expecting `re.compile('A{600}B{600}')`; buffer shows `cp /flash/large.txt /flash/large-copy.txt` with no prompt/output (large copy stalled).
  - `test_rmdir_only_removes_empty_directories` — `TIMEOUT` expecting `directory must be empty`; buffer excerpt shows `mkdir /flash/nonempty-to-remove` while test was at/after `rmdir /flash/nonempty-to-remove` step, indicating stalled/desynchronized command flow.
- **Passing in same run:** `test_nslookup_reports_unavailable_network` (C command still registered) and all Berry FS/NET API tests, including `test_berry_filesystem_and_network_apis` (original issue not present).

## Comparison to earlier CI failures

- **Different from original issue:** `test_berry_filesystem_and_network_apis` is passing in the runs analyzed here.
- **Not identical to run 43175605236 (`95c4f65`):** that run had 2 failures (`cp` large-copy stall, `rmdir` non-empty stall) and `test_shell_alias_lifecycle` passed; this run adds back alias timeout and introduces `ping` as "Unknown command" due to script migration + test seeding gap.
- **Not identical to the earlier 3-failure blob log summarized below:** `test_ping_reports_unavailable_network` now fails because `ping` is unresolved as a script in the test image (migration side-effect), not because the C command returned an unexpected error string.

## Earlier CI evidence (job log, previously provided)

Source: GitHub Actions job log (provided by user), `Integration Tests (QEMU)`, merge ref `e5b22c1...`.

- **Result:** `3 failed, 28 passed, 31 warnings in 371.18s` (`test_shell_fs`)
- **Failures:**
  - `test_shell_alias_lifecycle` — `pexpect.exceptions.TIMEOUT: Not found "Unknown command: ll"` (timeout 10s) after `unalias ll` / `ll`.
    - CI `dut.log`: `/tmp/pytest-embedded/2026-11-12_07-14-07-541394/test_shell_alias_lifecycle/dut.log`
  - `test_cp_copies_without_overwriting` — `pexpect.exceptions.TIMEOUT: Not found "re.compile('A{600}B{600}')"` (timeout 10s) while expecting copied large-file contents after `cat /flash/large-copy.txt`.
    - CI `dut.log`: `/tmp/pytest-embedded/2026-11-12_07-14-07-541394/test_cp_copies_without_overwriting/dut.log`
  - `test_rmdir_only_removes_empty_directories` — `pexpect.exceptions.TIMEOUT: Not found "directory must be empty"` (timeout 10s).
    - CI `dut.log`: `/tmp/pytest-embedded/2026-11-12_07-14-07-541394/test_rmdir_only_removes_empty_directories/dut.log`
- **Passing in same run:** includes `test_berry_filesystem_and_network_apis` (original issue), indicating the original failure mode is resolved.
- **Boot notes observed in CI log:** LittleFS mount failures at startup (`esp_littlefs: mount failed, (-84)` / `Failed to initialize LittleFS`) followed by format/remount in the test app (also seen locally); not by itself conclusive as the regression cause.

## Local reproduction (Windows, ESP-IDF v6.1, QEMU)

Environment notes: `IDF_PATH=C:\esp\v6.1\esp-idf`, `IDF_TOOLS_PATH=C:\Espressif\tools`, QEMU on PATH (`C:\Espressif\tools\tools\qemu-xtensa\esp_develop_9.2.2_20260417\qemu\bin`), Python `C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe`. Dangling `qemu-system-xtensa` processes were found and terminated between runs.

Commands (representative):

```powershell
# build integration app
$py = 'C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe'
$env:IDF_PATH = 'C:\esp\v6.1\esp-idf'
$env:IDF_TOOLS_PATH = 'C:\Espressif\tools'
& $py scripts/integration_test.py build test_shell_fs

# run targeted tests
$q = 'C:\Espressif\tools\tools\qemu-xtensa\esp_develop_9.2.2_20260417\qemu\bin'
$env:PATH = $q + ';' + $env:PATH
& $py -m pytest 'integration_test/test_shell_fs/test_shell_fs.py' -k "test_shell_alias_lifecycle or test_cp_copies_without_overwriting or test_rmdir_only_removes_empty_directories" --target esp32 --embedded-services idf,qemu -v
```

Observed:

- **Targeted trio (HEAD firmware, before/after test prompt-sync edits):** `test_shell_alias_lifecycle` **PASSED** locally; `test_cp_copies_without_overwriting` and `test_rmdir_only_removes_empty_directories` **FAILED** with pexpect timeouts.
- **Local `dut.log` behavior:** in this environment, filesystem operations can block/hang (e.g. `cp /flash/large.txt /flash/large-copy.txt` not returning a prompt; `rmdir /flash/nonempty-to-remove` not emitting the expected `directory must be empty` error/prompt within the test flow), preventing completion of assertions. This may be environment-specific (Windows/QEMU/LittleFS interaction) and is not identical to all CI symptom details (e.g. CI reported `cp` reaching `cat`/content expectation in context, and CI alias failed while local alias passed).

## Relevant changes in scope

- `components/filesystem/src/vfs.c`: `vfs_list_dir` optimized to populate entries from `dirent` (`d_type`) without per-entry `stat()`; reports `size/mtime/ctime=0` (intentional perf/stability change for directory listing). Not on the direct `cp`/`rmdir`/alias command execution paths, but a notable FS behavior/metadata change.
- `integration_test/test_shell_fs/test_shell_fs.py` and `integration_test/test_shell_fs/main/test_shell_fs.c`: Berry FS/NET API test flow/scripts adjusted (original issue fixed; passes in CI).
- `components/shell/src/shell_fs_cmds.c`, `components/shell/src/shell_alias.c`: `cp/cat/rmdir` and alias persistence logic (no confirmed regression identified from static review alone).

## Actions taken

- Fetched and analyzed CI job logs (`gh run view 43175605236 --log-failed`) and downloaded CI artifact `integration-test-logs` to inspect per-test `dut.log` files.
- Confirmed from CI `dut.log` (run 43175605236, `2026-11-12_07-38-44-139949`):
  - `test_cp_copies_without_overwriting`: boot/seed succeeded, small copy+cat succeeded, then `cp /flash/large.txt /flash/large-copy.txt` was issued and **no prompt/output followed** (command stalled) before timeout expecting copied content.
  - `test_rmdir_only_removes_empty_directories`: empty-dir `mkdir`/`rmdir` succeeded, `mkdir /flash/nonempty-to-remove` and `touch /flash/nonempty-to-remove/file.txt` succeeded, then `rmdir /flash/nonempty-to-remove` was issued and **no `directory must be empty`/prompt followed** (command stalled).
- Analyzed newly provided CI job log blob (checkout `944cf49`, 2026-11-12): **4 failures** (`test_shell_alias_lifecycle`, `test_ping_reports_unavailable_network`, `test_cp_copies_without_overwriting`, `test_rmdir_only_removes_empty_directories`). Identified `ping` failure as **migration/fixture gap**: native `ping` removed in favor of Berry script command (`shell_network_cmds.c` now registers only `nslookup`; scripts resolve by name via `shell_script_try_exec`), but `integration_test/test_shell_fs/main/test_shell_fs.c` did not seed `ping.be`, causing `Unknown command: ping example.com`.
- Reproduced targeted failures locally (partial alignment), inspected local `dut.log` outputs under `%TEMP%\pytest-embedded\...`.
- Instrumented and experimentally modified `vfs_copy_file` to localize copy-path behavior; restored and then reworked implementation as a targeted fix (see below).
- Applied prompt-synchronization fixes in `integration_test/test_shell_fs/test_shell_fs.py`:
  - `test_cp_copies_without_overwriting`: added `dut.expect(PROMPT_RE)` after `cat /flash/copied.txt` output.
  - `test_rmdir_only_removes_empty_directories`: added `dut.expect(PROMPT_RE)` after the expected `directory must be empty` error.
- Reworked `components/filesystem/src/vfs.c:vfs_copy_file` to stream source via `vfs_read_file_stream` (same read path as `cat`) and write via stdio, with existence/type checks via `vfs_exists`/`vfs_is_directory` and resolved-destination handling; avoids prior POSIX `open/fstat/read/write` copy path implicated in the observed large-copy stall.
- Updated `components/shell/src/shell_fs_cmds.c:cmd_rmdir` to detect non-empty directories via `vfs_list_dir` and report `rmdir: failed to remove '...' (directory must be empty)` **before** calling `vfs_remove`, avoiding reliance on a potentially blocking `rmdir()` syscall on non-empty LittleFS directories.
- Updated `integration_test/test_shell_fs/main/test_shell_fs.c` to seed `/flash/ping.be` (Berry `run(args)` script using `net.ping`, matching migrated command contract/expected failure output) so `test_ping_reports_unavailable_network` can resolve and execute `ping` as a script in the test environment.

## Open questions / gaps

- **Local validation blocked:** on this Windows/QEMU environment, the same targeted tests still stall after the fixes (`test_cp_copies_without_overwriting` hangs at large `cp`, `test_rmdir_only_removes_empty_directories` hangs at non-empty `rmdir`) despite rebuilds; this prevents confirming a green result locally and may reflect environment-specific LittleFS/QEMU behavior.
- CI re-validation is required to confirm the fixes resolve run 43175605236 symptoms (`test_cp_copies_without_overwriting`, `test_rmdir_only_removes_empty_directories`) on Linux/QEMU.
- Earlier blob-log report of `test_shell_alias_lifecycle` failing was not observed in run 43175605236 (it passed there) and did not reproduce locally; keep monitoring for flakiness.
- Whether `vfs_list_dir` metadata semantics (`d_type`-only `is_dir`, zeroed size/timestamps) have indirect effects on other consumers remains unproven.

## Recommended next steps

1. Obtain and inspect CI `dut.log` artifacts for the three failing tests to identify the exact failure point and output.
2. Reproduce on a CI-like Linux/ESP-IDF v6.1 + QEMU environment: verify large-file `cp` completion/content, `rmdir` non-empty error emission, and alias unalias behavior under the pushed firmware.
3. Implement the confirmed fix (firmware and/or test), rebuild, and rerun affected `test_shell_fs` cases serially (and full suite as appropriate).
4. Re-run required follow-up checks as applicable (`make build`, `make test`, `make lint-db`, `make lint`, `make format-check`, integration tests) and clean up dangling QEMU processes between runs.

## Validation log

- **CI (earlier provided blob log, `e5b22c1...` context):** `Integration Tests (QEMU)` / `test_shell_fs` — **3 failed, 28 passed** (`test_shell_alias_lifecycle`, `test_cp_copies_without_overwriting`, `test_rmdir_only_removes_empty_directories` failed; `test_berry_filesystem_and_network_apis` passed).
- **CI run 43175605236** (`feat/r14-ts-diags`, head `95c4f65`): **2 failed, 29 passed, 31 warnings** — `test_cp_copies_without_overwriting` stalled at large `cp`, `test_rmdir_only_removes_empty_directories` stalled at non-empty `rmdir`; `test_shell_alias_lifecycle` passed.
- **CI (latest provided blob log, head `944cf49` merge of `8c181d8` ping migration):** **4 failed, 27 passed, 31 warnings** — `test_shell_alias_lifecycle` (timeout expecting `Unknown command: ll`), `test_ping_reports_unavailable_network` (`Unknown command: ping example.com` because `/flash/ping.be` was not seeded in test image after C `ping` removal), `test_cp_copies_without_overwriting` (large `cp` stall), `test_rmdir_only_removes_empty_directories` (stall/desync around non-empty `rmdir`). `test_nslookup_reports_unavailable_network` and Berry API tests passed.
- **Local (Windows, ESP-IDF v6.1, QEMU):** not revalidated in this turn after seeding `ping.be`; prior local runs on this host were unreliable for the known `cp`/`rmdir` stall cases (environment-specific LittleFS/QEMU behavior). Automated local validation of the ping fix and remaining stall failures is pending/blocked here.

## Follow-up / next steps

1. Build `integration_test/test_shell_fs` and run CI (Linux/QEMU) with `/flash/ping.be` seeded in the test app to confirm `test_ping_reports_unavailable_network` passes post-migration; verify `ping` resolves from `/flash` in cwd.
2. Inspect CI `dut.log` artifacts for `test_shell_alias_lifecycle`, `test_cp_copies_without_overwriting`, and `test_rmdir_only_removes_empty_directories` if they still fail. For alias, confirm whether `unalias` command/dispatch or alias expansion state is incorrect vs prompt/output synchronization. For `cp`/`rmdir`, determine if stalls persist despite `vfs_copy_file` streaming and pre-`rmdir` `vfs_list_dir` emptiness check.
3. If stalls persist in CI, add targeted instrumentation or lower-level hardening (LittleFS/VFS call paths) and/or adjust test strategy only if coverage can be preserved and root cause is understood.
4. After integration tests are green, run required checks per repo policy and environment availability (`make format-check`, `make test`, `make build`, `make lint-db`/`make lint`, integration suite as applicable) and record exact outcomes; if ESP-IDF/QEMU/tooling is unavailable locally, document blockers and rerun commands for the user.
