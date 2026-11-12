# 2026-11-12: `test_shell_fs` CI regression investigation

## Summary

- **Question:** Is the fresh CI failure the same as the original `test_berry_filesystem_and_network_apis` failure?
- **Answer:** **No.** The original Berry FS/NET integration hang/timeout is no longer the failing case in the reported CI run; a different set of `test_shell_fs` regressions failed.
- **Status:** Investigation in progress. Prompt-synchronization fixes were applied to affected integration tests, but the CI-specific root cause is not yet confirmed and local QEMU/LittleFS runs in this environment show blocking FS behavior that prevents reliable validation.

## CI evidence (job log)

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
- Reproduced targeted failures locally (partial alignment), inspected local `dut.log` outputs under `%TEMP%\pytest-embedded\...`.
- Instrumented and experimentally modified `vfs_copy_file` to localize copy-path behavior; restored and then reworked implementation as a targeted fix (see below).
- Applied prompt-synchronization fixes in `integration_test/test_shell_fs/test_shell_fs.py`:
  - `test_cp_copies_without_overwriting`: added `dut.expect(PROMPT_RE)` after `cat /flash/copied.txt` output.
  - `test_rmdir_only_removes_empty_directories`: added `dut.expect(PROMPT_RE)` after the expected `directory must be empty` error.
- Reworked `components/filesystem/src/vfs.c:vfs_copy_file` to stream source via `vfs_read_file_stream` (same read path as `cat`) and write via stdio, with existence/type checks via `vfs_exists`/`vfs_is_directory` and resolved-destination handling; avoids prior POSIX `open/fstat/read/write` copy path implicated in the observed large-copy stall.
- Updated `components/shell/src/shell_fs_cmds.c:cmd_rmdir` to detect non-empty directories via `vfs_list_dir` and report `rmdir: failed to remove '...' (directory must be empty)` **before** calling `vfs_remove`, avoiding reliance on a potentially blocking `rmdir()` syscall on non-empty LittleFS directories.

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

- **CI (earlier provided blob log):** `Integration Tests (QEMU)` / `test_shell_fs` — **3 failed, 28 passed** (`test_shell_alias_lifecycle`, `test_cp_copies_without_overwriting`, `test_rmdir_only_removes_empty_directories` failed; `test_berry_filesystem_and_network_apis` passed).
- **CI run 43175605236** (`feat/r14-ts-diags`, head `95c4f65`, job `Integration Tests (QEMU)`, logs via `gh run view --log-failed`): **2 failed, 29 passed, 31 warnings** in `test_shell_fs` — `test_cp_copies_without_overwriting` timed out expecting `re.compile('A{600}B{600}')` with buffer `COS/flash> cp /flash/large.txt /flash/large-copy.txt`; `test_rmdir_only_removes_empty_directories` timed out expecting `directory must be empty` with buffer showing mkdir/touch/rmdir sequence issued. `test_shell_alias_lifecycle` **PASSED** in this run.
- **Local (targeted, representative):** before fixes, `test_shell_alias_lifecycle` **PASS**, `test_cp_copies_without_overwriting` **FAIL** (timeout), `test_rmdir_only_removes_empty_directories` **FAIL** (timeout). After applying prompt-sync/test + `vfs_copy_file` + `cmd_rmdir` fixes and rebuilding `test_shell_fs`, the same targeted run still produced **1 PASS / 2 FAIL** in this Windows/QEMU environment with command stalls in `dut.log`; reliable green validation pending (CI-like environment required).

## Follow-up / next steps

1. Push or run CI with current fixes and re-validate `test_shell_fs` (especially `test_cp_copies_without_overwriting` and `test_rmdir_only_removes_empty_directories`) in the CI Linux/QEMU environment; inspect uploaded `dut.log` artifacts if failures persist.
2. If cp still stalls in CI, investigate lower-level LittleFS/VFS write behavior for the seeded large-file copy path and consider alternative copy/test strategies that preserve coverage without triggering the hang.
3. If rmdir still stalls in CI, add targeted tracing around `cmd_rmdir`/`vfs_list_dir`/`vfs_remove` to determine which FS call blocks and harden accordingly.
4. After green integration tests, run required checks per repo policy: `make format-check`, `make test`, `make build`, `make lint-db`/`make lint` (as environment allows), and any other scope-relevant checks; document exact outcomes.
