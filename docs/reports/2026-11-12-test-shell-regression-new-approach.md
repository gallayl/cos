# Fix QEMU Integration Test Failures: LittleFS / VFS Investigation

## Context

GitHub Actions run:
https://github.com/gallayl/cos/actions/runs/43188508915/job/131081624009?pr=22

- PR: #22
- Tested commit: `b85e562`
- Merge commit: `e39e2bb`
- Previous relatively good baseline: `95c4f65`
- Result: 18 passed, 13 failed out of 31 tests.
- Failing suite: `test_shell_fs`
- Runtime: approximately 26 minutes.

The firmware boots successfully, initializes the shell, and reaches `COS/flash>`. The failures occur during subsequent filesystem operations.

## 1. Main Hypothesis

Investigate a possible LittleFS / ESP-IDF VFS / stdio interaction issue, potentially triggered by filesystem initialization, remounting, or Berry runtime initialization.

This is a hypothesis, NOT a confirmed root cause.

Important observations:

- Local instrumentation showed `test_touch_and_ls` and `test_berry_filesystem_and_network_apis` blocking inside:

  ```c
  FILE *f = fopen(real_path, "w");
  ```

- No subsequent `fwrite`, `fclose`, or function-exit log appeared.
- A watchdog backtrace in `test_mv_moves_file` included:

  ```text
  lfs_dir_getslice
    -> lfs_dir_getinfo
      -> lfs_stat
        -> vfs_littlefs_fstat
          -> esp_vfs_fstat
            -> fstat
              -> __bufio_buffer_allocate_locked
                -> fread
                  -> vfs_read_file
                    -> Berry runtime
                      -> shell_berry_run_autoexec
  ```

- Autoexec eventually completes and the shell becomes ready.
- Multiple seemingly unrelated shell commands use the same underlying filesystem functions.

## 2. Priority 1: Investigate Filesystem Initialization

Inspect the complete lifecycle:

```text
app_main()
  -> init_nvs()
  -> filesystem_init()
  -> seed_test_files()
  -> shell_init()
  -> SHELL_READY
```

Current implementation reportedly has the following behavior:

- `flash_init()` attempts to mount LittleFS.
- `format_if_mount_failed = 0`.
- Initial mount can fail with error `-84`.
- Initialization may still return success.
- `seed_test_files()` subsequently calls `flash_format()`.
- Formatting and mounting happen after the initial failed mount.

The CI log contains:

```text
Corrupted dir pair at {0x0, 0x1}
esp_littlefs: mount failed, (-84)
```

This may simply indicate an initially unformatted partition, but the initialization sequence should be simplified and tested.

### Proposed experiment

For the integration-test environment only:

1. Format the test partition BEFORE the first mount attempt.
2. Mount LittleFS once.
3. Remove the formatting operation from `seed_test_files()`.
4. Make `seed_test_files()` responsible only for creating test fixtures.
5. Verify that no duplicate or conflicting filesystem registration occurs.

Conceptual initialization:

```c
void app_main(void)
{
    init_nvs();

    ESP_ERROR_CHECK(flash_format());
    ESP_ERROR_CHECK(filesystem_init());

    seed_test_files();

    ESP_ERROR_CHECK(shell_init());

    ESP_LOGI(TAG, "SHELL_READY");
}
```

IMPORTANT:

- Verify the actual behavior and contracts of `flash_format()` and `filesystem_init()` before implementing this.
- The example assumes formatting leaves the filesystem mounted and that the subsequent initialization is safe.
- Do not introduce production behavior that automatically formats user data after a mount failure.
- If appropriate, compare this experiment with a mount configuration using `format_if_mount_failed = 1`.

## 3. Priority 2: Add Minimal Filesystem Isolation Tests

Add temporary diagnostics that bypass the shell and Berry entirely.

Test direct stdio operations immediately after filesystem initialization:

```c
static void test_littlefs_write(void)
{
    ESP_LOGI(TAG, "FS_TEST: before fopen");

    FILE *file = fopen("/littlefs/probe.txt", "w");

    ESP_LOGI(TAG, "FS_TEST: after fopen");

    if (file == NULL)
    {
        ESP_LOGE(TAG, "FS_TEST: fopen failed");
        return;
    }

    ESP_LOGI(TAG, "FS_TEST: before fwrite");

    const char *content = "LittleFS test";
    size_t written = fwrite(content, 1, strlen(content), file);

    ESP_LOGI(TAG, "FS_TEST: after fwrite (%u)", (unsigned)written);

    fclose(file);

    ESP_LOGI(TAG, "FS_TEST: completed");
}
```

Run the same operation under these conditions:

| Scenario                                             | Purpose                                                            |
| ---------------------------------------------------- | ------------------------------------------------------------------ |
| After filesystem initialization, before shell_init() | Verify basic LittleFS operation                                    |
| After shell_init()                                   | Determine whether shell initialization affects filesystem behavior |
| With Berry autoexec disabled                         | Determine whether autoexec contributes to the issue                |

Also test:

- `open()` / `close()` (POSIX)
- `fopen()` / `fwrite()` / `fclose()` (stdio)
- `stat()`
- `mkdir()`
- `unlink()`
- `fopen()` in read mode.

Log entry and exit around each operation to identify the exact blocking call.

## 4. Priority 3: Inspect Shared VFS Implementations

Inspect these functions and their call chains:

| Function           | Consumers                                       |
| ------------------ | ----------------------------------------------- |
| `vfs_write_file()` | `touch`, aliases, Berry `fs.write_text()`, `cp` |
| `vfs_mkdir()`      | `mkdir`, nested directory creation              |
| `vfs_copy_file()`  | `cp`                                            |
| `vfs_read_file()`  | Berry autoexec and file-reading APIs            |
| `save_aliases()`   | Shell alias lifecycle                           |

Pay particular attention to:

- Filesystem locking and possible recursive lock acquisition.
- File descriptor lifecycle and cleanup.
- LittleFS/VFS mount state.
- Concurrent or re-entrant filesystem access.
- Interactions between Berry runtime initialization and filesystem access.
- Whether any FILE handles remain open after autoexec execution.
- Error paths that leave filesystem resources locked.

Do not rewrite individual shell commands unless their own implementation is demonstrably faulty.

## 5. Priority 4: Preserve Existing mkdir Improvement

The newer `vfs_mkdir()` implementation attempts `mkdir()` first and performs additional checks only when necessary.

This change appears to have resolved:

- `test_mkdir_and_cd`
- `test_nested_mkdir`

Keep this improvement unless further testing identifies a regression.

However, it does not resolve the underlying filesystem write issue.

## 6. Priority 5: Bisect the Regression

Use the following commits:

- Relatively good baseline: `95c4f65`
- Suspect commit: `b85e562`

Identify the first commit that introduces the failure.

Use these two tests as primary regression indicators:

- `test_touch_and_ls`
- `test_berry_filesystem_and_network_apis`

Suggested command:

```powershell
pytest integration_test/test_shell_fs/test_shell_fs.py `
  -k "test_touch_and_ls or test_berry_filesystem_and_network_apis" `
  --target esp32 `
  --embedded-services idf,qemu -v
```

Do not assume the regression is limited to filesystem-related commits. Include Berry runtime and shell initialization changes in the bisect.

## 7. Do Not Waste Time On

The following are not currently considered root causes:

- GitHub Actions Node.js 20 deprecation warnings.
- Ubuntu 26 migration warnings.
- Increasing command timeouts.
- Rewriting individual shell commands without identifying the shared failure.
- The missing `ping.be` fixture, which was already addressed.
- The ping script migration, which passes in this run.

A command blocked inside `fopen()` will not become healthy merely because it gets another 20 seconds to contemplate its existence.

## 8. Acceptance Criteria

The implementation is complete when:

- [ ] LittleFS initialization has a clear, deterministic format/mount lifecycle.
- [ ] The filesystem can be written to before shell initialization.
- [ ] The filesystem can be written to after shell initialization.
- [ ] Berry autoexec does not leave the filesystem in a blocked or inconsistent state.
- [ ] `fopen`, `fwrite`, `fclose` and POSIX filesystem operations complete successfully.
- [ ] All previously failing `test_shell_fs` tests pass.
- [ ] Existing passing filesystem, shell, Berry and network tests remain passing.
- [ ] Full QEMU integration tests pass in GitHub Actions.
- [ ] Temporary debugging instrumentation is removed or converted into useful regression tests.

## Implementation Instructions

Work incrementally.

1. Inspect the actual implementations and existing test setup.
2. Establish a reproducible failing test locally.
3. Implement the filesystem initialization experiment.
4. Run the minimal filesystem isolation tests.
5. Identify and fix the actual root cause.
6. Add a regression test for the discovered issue.
7. Run the complete integration suite.

Do not apply speculative fixes to multiple subsystems simultaneously.

**Expected deliverable:** A focused code change with a proven root cause, regression coverage, and passing QEMU integration tests. Document the exact cause and why the fix resolves it. If the initialization experiment does not resolve the issue, revert the experiment and continue investigating rather than retaining an unrelated change.
