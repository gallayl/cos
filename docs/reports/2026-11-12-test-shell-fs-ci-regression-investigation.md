# 2026-11-12: `test_shell_fs` CI regression investigation

## Summary

- **Question:** Is the fresh CI failure the same as the original `test_berry_filesystem_and_network_apis` failure?
- **Answer:** **No, but related to filesystem/Berry behavior.** Run 43188508915 (`e39e2bb`) shows **13 `test_shell_fs` failures** where the shell reaches prompt/ready state, then hangs on early post-prompt filesystem-mutating operations (or mid-Berry FS API script), producing prompt timeouts. A task WDT was observed in LittleFS/VFS/Berry autoexec read path (`test_mv_moves_file`), indicating LittleFS/VFS stalls/deadlock risk.
- **Status:** Investigation in progress (updated 2026-11-12 after analyzing `integration-test-logs` artifact for run 43188508915). Root cause not yet fixed; likely in LittleFS/VFS/filesystem command interactions under this test/image flow. CI (Linux/QEMU) remains the authoritative validation target.

## Latest CI evidence (fresh run, 2026-11-12)

Source: GitHub Actions job `Integration Tests (QEMU)`, container `espressif/idf:v6.1`, checkout `e39e2bb` (`Merge b85e562d3f678e76e21fd219f4d6faa07224763a into a9da155b616221fa634a0fa3a799117ce69f5c7d`), run 43188508915.

- **Result:** **13 failed, 18 passed, 31 warnings** in `test_shell_fs` (pytest `--embedded-services idf,qemu`).
- **Failed tests (by pytest):**
  - `test_touch_and_ls` — TIMEOUT waiting for prompt `re.compile('COS.+>')`
  - `test_shell_alias_lifecycle` — TIMEOUT waiting for prompt `re.compile('COS.+>')`
  - `test_berry_filesystem_and_network_apis` — TIMEOUT waiting for `FS_TEXT_OK` (Berry FS/NET path)
  - `test_mkdir_and_cd` — TIMEOUT waiting for prompt
  - `test_nested_mkdir` — TIMEOUT waiting for prompt
  - `test_cp_copies_without_overwriting` — TIMEOUT waiting for prompt
  - `test_cp_rejects_directory_copy` — TIMEOUT waiting for prompt
  - `test_mv_moves_file` — TIMEOUT waiting for prompt (task WDT/backtrace observed)
  - `test_rmdir_only_removes_empty_directories` — TIMEOUT waiting for prompt
  - `test_cd_relative` — TIMEOUT waiting for prompt
  - `test_rm_file` — TIMEOUT waiting for prompt
  - `test_alias_md` — TIMEOUT waiting for prompt
  - `test_alias_del` — TIMEOUT waiting for prompt
- **Passing (notable):** `test_shell_boots`, `test_pwd_default`, `test_ls_shows_seeded_files`, `test_cat_with_content`, `test_cat_streams_large_file`, `test_autocomplete_*`, `test_help_lists_registered_commands`, `test_ping_reports_unavailable_network` (PASSED), `test_nslookup_reports_unavailable_network` (PASSED), `test_berry_console_read_line_*` (PASSED), `test_hexdump` (PASSED), `test_info_command`, `test_alias_dir`, `test_alias_type`, `test_cd_nonexistent`, `test_cat_nonexistent`, `test_rm_nonexistent`.
- **Crash/backtrace evidence (CI artifact logs):** `pytest-embedded-logs/.../test_mv_moves_file/dut.log` shows task WDT timeout with backtrace through LittleFS (`lfs_dir_getslice/lfs_dir_getinfo/lfs_stat`), VFS (`vfs_littlefs_fstat`, `esp_vfs_fstat`, `fstat`), stdio buffering (`__bufio_buffer_allocate_locked/fread`), and up into Berry runtime (`vfs_read_file` called from Berry runtime reading virtual/file content) and `shell_berry_run_autoexec` during shell init / `app_main`. Despite the WDT, autoexec completed afterward and the shell reached ready state; the subsequent `touch` command still hung.

### Log analysis (artifact `integration-test-logs`, run 43188508915)

- **Boot/ready state:** In all inspected failing `test_shell_fs` cases, the app boots, initial LittleFS mount fails with `esp_littlefs: ... error: Corrupted dir pair at {0x0, 0x1}` / `mount failed (-84)`, then the test app formats/remounts successfully, seeds files, runs autoexec (`AUTOEXEC_OK`), prints `Shell ready` / `SHELL_READY`, and shows prompt `COS/flash>`. The failures occur **after prompt**, not during startup.
- **Hang point:** each failing test stalls on the first (or early) filesystem-mutating command/script operation sent after prompt, with no output and no prompt return:
  - `test_touch_and_ls`: hung after `touch /flash/newfile.txt`
  - `test_shell_alias_lifecycle`: hung after `alias ll ls`
  - `test_mkdir_and_cd`, `test_nested_mkdir`, `test_cd_relative`: hung after `mkdir ...`
  - `test_cp_copies_without_overwriting`, `test_cp_rejects_directory_copy`: hung after `cp ...` / setup `mkdir ...`
  - `test_mv_moves_file`: hung after `touch /flash/move-source.txt` (after prior WDT during autoexec)
  - `test_rmdir_only_removes_empty_directories`: hung after `mkdir /flash/empty-to-remove`
  - `test_rm_file`, `test_alias_del`: hung after `touch ...`
  - `test_alias_md`: hung after `md /flash/mdtest`
  - `test_berry_filesystem_and_network_apis`: `berry run /flash/berry_api.be` progressed through `S1`, `FS_LIST_OK`, `FS_EMPTY_OK`, `FS_RELATIVE_OK`, `FS_PARENT_OK`, `FS_INVALID_PATH_OK`, then hung before `W1`/`FS_TEXT_OK` (next script steps are `fs.write_text/read_text/size/exists/is_file/is_dir`).
- **WDT/backtraces:** task WDT only observed in `test_mv_moves_file/dut.log` (LittleFS/VFS/fread/`vfs_read_file`/Berry autoexec path). No other `task_wdt`, panic, abort, or Guru Meditation strings found in scanned `dut.log` files.
- **Implication:** likely a LittleFS/VFS deadlock or stall triggered by specific write/create/stat-related operations in this environment/image state (possibly exacerbated by mount-fail→format→remount/seed sequence), rather than a pure prompt-sync or command-dispatch regression. Seed-time writes complete, but interactive FS ops later can wedge the system.

## Comparison to earlier CI failures

- **Different from earlier stable-ish runs:** this run has 13 failures with widespread prompt timeouts (`COS.+>` not reached) rather than the 2–4 targeted command-level timeouts seen previously.
- **Berry API test:** `test_berry_filesystem_and_network_apis` failed here with `TIMEOUT` waiting for `FS_TEXT_OK` (previously passed in runs 43175605236/944cf49 blob log); behavior is consistent with shell/FS/Berry initialization or execution stall rather than the original isolated hang alone.
- **Ping:** `test_ping_reports_unavailable_network` **passed** in this run (unlike 944cf49 blob log where `ping` was unresolved due to missing seeded `ping.be`).
- **Backtrace signal:** task WDT timeout in `test_mv_moves_file` shows LittleFS/VFS/stdio/Berry-runtime path (`lfs_*` → `vfs_littlefs_fstat` → `fread` → `vfs_read_file` → Berry runtime/`shell_berry_run_autoexec`), suggesting filesystem or Berry init/read stall as a possible shared cause for prompt loss across tests.

## Earlier CI evidence

### Run at head `944cf49` (blob log, 2026-11-12, previously analyzed)

Source: user-provided GitHub Actions job log blob (job `Integration Tests (QEMU)`, container `espressif/idf:v6.1`), checkout `944cf49` (`Merge 8c181d814e66e4c55d57701c0507540402d569cc into a9da155...`).

- **Result:** `4 failed, 27 passed, 31 warnings` in `test_shell_fs`.
- **Failures:**
  - `test_shell_alias_lifecycle` — `TIMEOUT` expecting `Unknown command: ll` after `unalias ll` / `ll`. Buffer: `unalias ll` (no "Unknown command" line observed).
  - `test_ping_reports_unavailable_network` — `TIMEOUT` expecting `ping: example.com failed (ESP_ERR_INVALID_STATE)`. Buffer showed `I (...) main_task: Returned from app_main()` followed by `ping example.com` and `Unknown command: ping example.com` (duplicate prompts). `ping` was script-provided after commit `8c181d8`, but the integration test app did not seed `/flash/ping.be`, so script dispatch could not resolve `ping`.
  - `test_cp_copies_without_overwriting` — `TIMEOUT` expecting `re.compile('A{600}B{600}')`; buffer shows `cp /flash/large.txt /flash/large-copy.txt` with no prompt/output (large copy stalled).
  - `test_rmdir_only_removes_empty_directories` — `TIMEOUT` expecting `directory must be empty`; buffer excerpt showed stalled/desynchronized command flow.
- **Passing in same run:** `test_nslookup_reports_unavailable_network` and all Berry FS/NET API tests, including `test_berry_filesystem_and_network_apis`.

### Run `43175605236` (`95c4f65`)

- **Result:** `2 failed, 29 passed, 31 warnings` — `test_cp_copies_without_overwriting` stalled at large `cp`, `test_rmdir_only_removes_empty_directories` stalled at non-empty `rmdir`; `test_shell_alias_lifecycle` passed.

### Blob log `e5b22c1...` (earlier)

- **Result:** `3 failed, 28 passed, 31 warnings` — `test_shell_alias_lifecycle`, `test_cp_copies_without_overwriting`, `test_rmdir_only_removes_empty_directories` failed; `test_berry_filesystem_and_network_apis` passed. Boot notes: LittleFS mount failures at startup (`esp_littlefs: mount failed, (-84)` / `Failed to initialize LittleFS`) followed by format/remount in the test app.

## Local reproduction (Windows, ESP-IDF v6.1, QEMU)

Environment notes: `IDF_PATH=C:\esp\v6.1\esp-idf`, `IDF_TOOLS_PATH=C:\Espressif\tools`, QEMU on PATH (`C:\Espressif\tools\tools\qemu-xtensa\esp_develop_9.2.2_20260417\qemu\bin`), Python `C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe`.

Commands (representative):

```powershell
$py = 'C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe'
$env:IDF_PATH = 'C:\esp\v6.1\esp-idf'
$env:IDF_TOOLS_PATH = 'C:\Espressif\tools'
$q = 'C:\Espressif\tools\tools\qemu-xtensa\esp_develop_9.2.2_20260417\qemu\bin'
$env:PATH = $q + ';' + $env:PATH
& $py scripts/integration_test.py build test_shell_fs
& $py -m pytest 'integration_test/test_shell_fs/test_shell_fs.py' -k 'test_touch_and_ls' --target esp32 --embedded-services idf,qemu -v
& $py -m pytest 'integration_test/test_shell_fs/test_shell_fs.py' -k 'test_berry_filesystem_and_network_apis' --target esp32 --embedded-services idf,qemu -v
```

Observed (2026-11-12, HEAD `dec81e8-dirty`, local investigation workspace):

- **Reproduced CI pattern:** `test_touch_and_ls` reached `SHELL_READY`/prompt, then hung after `touch /flash/newfile.txt` (timeout, no prompt). `test_berry_filesystem_and_network_apis` reached prompt, ran `berry run /flash/berry_api.be`, emitted `S1`, `FS_LIST_OK`, `FS_EMPTY_OK`, `FS_RELATIVE_OK`, `FS_PARENT_OK`, `FS_INVALID_PATH_OK`, then hung before `W1`/`FS_TEXT_OK`.
- This enables local repro investigation for this specific regression (unlike earlier Windows/QEMU runs that only partially matched prior stall cases). Clean up dangling `qemu-system-xtensa` processes between runs.

## Relevant changes in scope

- **`95c4f65..b85e562` diff (filesystem/shell/test):** `components/filesystem/src/vfs.c`, `components/shell/src/shell_fs_cmds.c`, `components/shell/src/shell_network_cmds.c`, `integration_test/test_shell_fs/main/test_shell_fs.c`, `integration_test/test_shell_fs/test_shell_fs.py` (82 insertions, 107 deletions). No `components/filesystem/src/flash.c` or Berry runtime changes in this specific range.
  - `vfs.c`: `vfs_list_dir` no longer stats each entry (uses `dirent.d_type`, reports `size/mtime/ctime=0`); `vfs_copy_file` rewritten to refuse overwrite via `vfs_exists`, check source via `vfs_exists/vfs_is_directory`, and stream copy via `vfs_read_file_stream` + `fopen/fwrite/fclose`; `vfs_mkdir` adds early `vfs_is_directory/vfs_exists` checks and tolerant `EEXIST` handling.
  - `shell_fs_cmds.c`: `cmd_rmdir` now checks emptiness via `vfs_list_dir` before `vfs_remove`.
  - `test_shell_fs.c/.py`: test seeding/prompt-sync adjustments including seeding `/flash/ping.be` (simplified inline script) for migrated ping command.
- **`a9da155..b85e562` (broader PR/merge context):** large changes across Berry runtime, shell, filesystem, VFS, etc.; `flash.c` only minor bool/int init style in `esp_vfs_littlefs_conf_t`; `vfs.c` includes the same list/copy/mkdir changes plus other differences vs `a9da155`.
- **Related plan:** `docs/plan/2026-11-12-integration-test-script-deduplication.md` — relevant as a **risk/constraint** for any script-seeding changes (canonical `storage/flash_data` scripts differ from current inline test scripts; naive replacement could break autoexec/contracts), but not confirmed as direct cause of run 43188508915.

## Diff review notes (`95c4f65..b85e562`, step 1)

- No single obvious culprit for **post-prompt hangs on simple mutating ops** (`touch/mkdir/rm/alias write`) was identified in this range alone: `flash.c` unchanged, `vfs_list_dir` avoids per-entry `stat` (less syscall pressure), `vfs_mkdir/copy_file` add checks but still rely on standard VFS/LittleFS calls.
- `vfs_mkdir` now performs `vfs_is_directory`/`vfs_exists` (each may `stat`) before `mkdir`; could amplify stalls if underlying FS is already wedged, but unlikely to be the primary trigger.
- Given logs show format/remount succeeds and seed writes complete but later interactive ops hang (and one task-WDT in `lfs_*`/`vfs_littlefs_fstat`/`fread` during Berry file read), suspect **LittleFS/VFS state, locking, or component/runtime behavior** in this environment/image flow (possibly introduced outside this narrow diff or exposed by it) rather than only command-level logic.

## Repro investigation progress (2026-11-12, local Windows/QEMU)

- **Local environment available:** `C:\esp\v6.1\esp-idf`, `C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe`, QEMU xtensa (`esp_develop_9.2.2_20260417`) confirmed present.
- **Build/test loop:** `python scripts/integration_test.py build test_shell_fs` and targeted pytest runs completed at HEAD `dec81e8-dirty`.
- **Baseline repro confirmed locally:**
  - `-k test_touch_and_ls` → **FAILED** (timeout). DUT reached `SHELL_READY`/prompt then hung after `touch /flash/newfile.txt`.
  - `-k test_berry_filesystem_and_network_apis` → **FAILED** (timeout waiting for `FS_TEXT_OK`). Hung after `FS_INVALID_PATH_OK`, before `fs.write_text`/`FS_TEXT_OK`.
- **Fix experiment (vfs_mkdir, uncommitted, kept):** `components/filesystem/src/vfs.c:vfs_mkdir` changed from pre-`stat` checks to `mkdir`-first + single `stat` on `EEXIST` (preserves semantics, avoids pre-stats on common create path).
  - Result: `-k 'test_mkdir_and_cd or test_nested_mkdir or test_touch_and_ls or test_berry_filesystem_and_network_apis'` → **2 passed, 2 failed** (`test_mkdir_and_cd`, `test_nested_mkdir` PASSED; `test_touch_and_ls`, `test_berry_filesystem_and_network_apis` still hung).
- **Step 1 instrumentation results (temporary; reverted from source after tracing):**
  - `test_berry_filesystem_and_network_apis`: hung at Berry `fs.write_text("./berry-note.txt","note")` with logs `DBG vfs_write_file enter /flash/berry-note.txt` → `DBG vfs_write_file fopen /littlefs/berry-note.txt` and **no fwrite/fclose/exit** → **blocked inside `fopen(real_path, "w")`**.
  - `test_touch_and_ls`: `vfs_exists` completed (`DBG vfs_exists enter/stat/exit ... 0`), then `vfs_write_file` entered and logged `DBG vfs_write_file fopen /littlefs/newfile.txt` with **no fwrite/fclose/exit** → **blocked inside `fopen(real_path, "w")`** after a successful non-existence `stat`.
- **Interpretation:** common blocker for reproduced post-prompt create/write cases is **`fopen(..., "w")` on LittleFS** (open-for-create/write path) after mount-fail (`-84`, corrupted dir pair) → format/remount → successful seed/autoexec. `stat` can still succeed shortly before the hang (as seen in touch), so the wedge is likely in LittleFS/esp_littlefs/VFS open/write/create state or locking under this QEMU/IDF flow.
- **Working tree:** `components/filesystem/src/vfs.c` contains the **`vfs_mkdir` improvement** (kept) with temporary instrumentation removed from source; `docs/reports/2026-11-12-test-shell-fs-ci-regression-investigation.md` updated.

## Mitigation attempts (2026-11-12, local)

- **`components/filesystem/src/flash.c` best-effort unregister before format:** added unconditional `(void)esp_vfs_littlefs_unregister(...)` before `esp_littlefs_format` to clear potential stale registration after failed mount; rebuilt and reran `-k 'test_touch_and_ls or test_berry_filesystem_and_network_apis'` → **still 2 failed** (same `fopen("w")` hang signatures; boot logged `E (...) esp_littlefs: Partition was never registered.` as expected for unregistered case). Change reverted to keep behavior/logs unchanged.
- **Partition erase before format (earlier experiment, per investigation notes):** not helpful / regressed behavior in tested form; not retained.

## Next focus / blockers

- **Root cause not fixed:** reproduced blocker remains **`fopen(..., "w")` on LittleFS** after mount-fail (`-84`, corrupted dir pair) → format/remount → successful seed/autoexec (confirmed for `test_touch_and_ls` and Berry `fs.write_text`). `vfs_mkdir` improvement reduced pre-stat pressure and **locally unblocked `test_mkdir_and_cd`/`test_nested_mkdir`** in targeted run, but other create/write paths still hang.
- **Needs further investigation (likely requires CI/Linux-QEMU for authoritative repro/bisect):** esp_littlefs/LittleFS/VFS behavior in QEMU under IDF v6.1 when formatting/remounting after initial corrupted mount; possible stale internal/VFS state, locking/reentrancy, or interaction with Berry runtime/shell command execution and post-autoexec operations. Bisect `95c4f65` → `b85e562`/`e39e2bb` and inspect LittleFS/esp_littlefs managed-component behavior/config in `integration_test/test_shell_fs`.
- **Finalize blocker:** cannot confirm full `test_shell_fs` recovery or CI fix from this Windows/QEMU environment alone; targeted local failures still reproduce. If available, run full integration suite in CI/Linux-QEMU (ESP-IDF v6.1) to validate any fix.

## Actions taken

- Fetched and analyzed CI job logs (`gh run view 43175605236 --log-failed`) and downloaded CI artifact `integration-test-logs` to inspect per-test `dut.log` files (earlier runs).
- Confirmed from CI `dut.log` (run 43175605236, `2026-11-12_07-38-44-139949`): `test_cp_copies_without_overwriting` stalled after `cp /flash/large.txt /flash/large-copy.txt`; `test_rmdir_only_removes_empty_directories` stalled after `rmdir /flash/nonempty-to-remove`.
- Analyzed blob log (checkout `944cf49`, 2026-11-12): **4 failures** including `test_ping_reports_unavailable_network` due to missing seeded `/flash/ping.be` after ping migration (script-based command). Updated `integration_test/test_shell_fs/main/test_shell_fs.c` to seed `ping.be` (earlier in investigation).
- **Fresh run 43188508915 analyzed (2026-11-12, head `e39e2bb`):** downloaded and inspected job log and provided `integration-test-logs` artifact (`pytest-embedded-logs/2026-11-12_09-20-11-974677/...`).
  - Result: **13 failed, 18 passed, 31 warnings** in `test_shell_fs`. Failures are prompt timeouts; `test_berry_filesystem_and_network_apis` timed out waiting for `FS_TEXT_OK`.
  - Inspected failing `dut.log` files: all reached boot → LittleFS mount fail (`Corrupted dir pair at {0x0, 0x1}`, `-84`) → format/remount → seed → autoexec `AUTOEXEC_OK` → `Shell ready`/prompt. Hangs occur **after prompt** on first/early FS-mutating command or mid-Berry FS script (`fs.list` OKs emitted, hang before `fs.write_text`/`FS_TEXT_OK`).
  - Scanned failing logs for `task_wdt`/panic/abort/Guru Meditation: only `test_mv_moves_file` had a task WDT backtrace in LittleFS/VFS/fread/`vfs_read_file`/Berry autoexec path; autoexec still completed and shell became ready, but later `touch` hung.
- **Local repro + targeted fix experiment (2026-11-12, Windows/QEMU, HEAD `dec81e8-dirty`):**
  - Built and reproduced failures locally: `test_touch_and_ls` hung after `touch /flash/newfile.txt`; `test_berry_filesystem_and_network_apis` hung before `FS_TEXT_OK` (after `FS_INVALID_PATH_OK`).
  - Modified `components/filesystem/src/vfs.c:vfs_mkdir` to avoid pre-`stat` calls (`vfs_is_directory`/`vfs_exists`) on the common path: attempt `mkdir` first, then on `EEXIST` perform a single `stat` to return `ESP_OK` for existing directories or `ESP_ERR_INVALID_STATE` for non-directory paths.
  - Re-tested: `-k 'test_mkdir_and_cd or test_nested_mkdir or test_touch_and_ls or test_berry_filesystem_and_network_apis'` → **`test_mkdir_and_cd` PASSED, `test_nested_mkdir` PASSED, `test_touch_and_ls` FAILED (hang), `test_berry_filesystem_and_network_apis` FAILED (hang at same point)**.
- Prior mitigation attempts (from earlier investigation): prompt-sync fixes in pytest, `vfs_copy_file` reworked to stream via `vfs_read_file_stream`, `cmd_rmdir` pre-check via `vfs_list_dir`, and `/flash/ping.be` seeding in test app. These do **not** explain the new widespread post-prompt FS-op stalls observed in e39e2bb.

## Open questions / gaps

- **Primary unknown:** why do LittleFS/VFS-backed operations hang after prompt in this run despite successful seed/autoexec? Suspects include LittleFS state/corruption handling after mount-fail→format→remount, VFS locking/deadlock, interaction with Berry FS APIs/runtime, or regressions in filesystem component/commands introduced on or before `e39e2bb`.
- **Trigger conditions:** hangs occur on create/write-like operations (`touch`, `mkdir`, `cp`, `rm`/`del`, alias writes, Berry `fs.write_text`/metadata ops) but `fs.list` calls in Berry script completed; need to pinpoint the exact API/syscall path that wedges.
- **Single vs multiple issues:** task WDT in `test_mv_moves_file` during autoexec read suggests a broader LittleFS/VFS stall class, but only one WDT was captured; other hangs may be silent deadlocks.
- **Environment reproducibility:** local Windows/QEMU environment previously unreliable for stall cases; CI (Linux/QEMU) is the authoritative signal.
- **Relation to prior fixes and deduplication plan:** earlier `cp`/`rmdir`/ping/alias/test-seeding fixes may be masked by this issue. If scripts/fixtures are changed later, `docs/plan/2026-11-12-integration-test-script-deduplication.md` constraints apply (avoid incompatible canonical script substitution).

## Recommended next steps

1. **Inspect CI `dut.log` artifacts** for several failed tests in run 43188508915 (especially `test_shell_boots` neighbors, `test_touch_and_ls`, `test_berry_filesystem_and_network_apis`, `test_mv_moves_file`) to confirm whether shell prompt ever appeared and capture common failure signatures.
2. **Correlate with code changes** between `944cf49`/`95c4f65` and `e39e2bb` (Berry runtime/shell Berry init, VFS/filesystem, LittleFS integration, test app autoexec scripts) to identify candidates causing `vfs_read_file`/Berry autoexec stalls.
3. **Reproduce in CI-like environment** (ESP-IDF v6.1, Linux/QEMU) if possible; otherwise iterate via targeted fixes and CI runs. Add temporary debug logging/instrumentation only if needed and removable.
4. **Implement and validate fix**, then rerun `integration_test/test_shell_fs` and related integration tests; verify `test_berry_filesystem_and_network_apis` and prompt-dependent FS tests recover.
5. **Run required checks** per repo policy as environment allows (`make format-check`, `make test`, `make build`, `make lint-db`/`make lint`, integration tests) and record exact outcomes/blockers.

## Validation log

- **Local (Windows, ESP-IDF v6.1, QEMU, 2026-11-12, HEAD `dec81e8-dirty` + uncommitted investigation changes incl. `components/filesystem/src/vfs.c` mkdir fix):**
  - `python scripts/integration_test.py build test_shell_fs` — **pass** (multiple iterations).
  - `pytest integration_test/test_shell_fs/test_shell_fs.py -k test_touch_and_ls --target esp32 --embedded-services idf,qemu` — **fail** (timeout, hung after `touch /flash/newfile.txt` post-prompt).
  - `pytest integration_test/test_shell_fs/test_shell_fs.py -k test_berry_filesystem_and_network_apis --target esp32 --embedded-services idf,qemu` — **fail** (timeout, hung before `FS_TEXT_OK` after `FS_INVALID_PATH_OK`).
  - `pytest integration_test/test_shell_fs/test_shell_fs.py -k 'test_mkdir_and_cd or test_nested_mkdir or test_touch_and_ls or test_berry_filesystem_and_network_apis' --target esp32 --embedded-services idf,qemu` — **2 passed, 2 failed** (`test_mkdir_and_cd`, `test_nested_mkdir` passed; `test_touch_and_ls`, `test_berry_filesystem_and_network_apis` failed with same `fopen("w")` hang signatures).
  - `pytest integration_test/test_shell_fs/test_shell_fs.py -k 'test_touch_and_ls or test_berry_filesystem_and_network_apis' --target esp32 --embedded-services idf,qemu` after `flash.c` best-effort unregister-before-format experiment — **2 failed** (same hang signatures); experiment reverted. **Full `test_shell_fs` suite and CI/Linux-QEMU not run locally.**
  - `make format-check` — **pass** (clang-format dry-run completed, exit 0).
  - `make test` — **pass** (host build + ctest **24/24 passed**, Python `unittest discover -s scripts -p "test_*.py"` **3 tests OK**).
- **Not run / blocked in this environment:**
  - `make build`, `make lint-db`, `make lint` — require ESP-IDF v6.1 build/lint environment setup aligned with CI (clang compile DB for lint); not completed as part of this finalize pass.
  - Full `test_shell_fs` integration suite and CI/Linux-QEMU validation — **required to confirm fix/regression status authoritatively**; local targeted repros still fail at `fopen("w")`.
- **CI run 43188508915** (PR #22 merge, head `e39e2bb`, 2026-11-12): `Integration Tests (QEMU)` / `test_shell_fs` — **13 failed, 18 passed, 31 warnings**. Failures: `test_touch_and_ls`, `test_shell_alias_lifecycle`, `test_berry_filesystem_and_network_apis`, `test_mkdir_and_cd`, `test_nested_mkdir`, `test_cp_copies_without_overwriting`, `test_cp_rejects_directory_copy`, `test_mv_moves_file`, `test_rmdir_only_removes_empty_directories`, `test_cd_relative`, `test_rm_file`, `test_alias_md`, `test_alias_del` (mostly prompt timeouts; Berry API timeout for `FS_TEXT_OK`). Task WDT backtrace observed in `test_mv_moves_file` dut log (LittleFS/VFS/Berry autoexec path).
- **CI (blob log, head `944cf49`, 2026-11-12):** **4 failed, 27 passed, 31 warnings** — `test_shell_alias_lifecycle`, `test_ping_reports_unavailable_network` (missing seeded `ping.be`), `test_cp_copies_without_overwriting`, `test_rmdir_only_removes_empty_directories`.
- **CI run 43175605236** (`95c4f65`): **2 failed, 29 passed, 31 warnings** — large `cp` stall, non-empty `rmdir` stall.
- **CI (blob log, `e5b22c1...`):** **3 failed, 28 passed, 31 warnings** — `test_shell_alias_lifecycle`, `test_cp_copies_without_overwriting`, `test_rmdir_only_removes_empty_directories`.

## Follow-up / next steps

1. Pull/download `integration-test-logs` artifact for run 43188508915 and review `dut.log` for representative failed tests to map prompt-absent vs command-stall cases.
2. Identify the regression introduced on/near `e39e2bb` (merge of `b85e562...` into `a9da155...`) affecting Berry autoexec or filesystem reads; prioritize fixing the task-WDT/LittleFS stall path.
3. If touching seeded/stock scripts as part of a fix, account for `docs/plan/2026-11-12-integration-test-script-deduplication.md`: do not blindly replace integration-test inline scripts with canonical `storage/flash_data` scripts when contracts/runtime dependencies differ (`cos_import`, `__cos_entry`, output/behavior). Classify as canonical-equivalent vs test-only fixture and validate accordingly.
4. After fix, rerun CI integration tests and update this report with root cause, code changes, and exact validation outcomes. If local ESP-IDF/QEMU validation cannot be completed on Windows, document required environment/setup and rerun commands for CI or a Linux host.
