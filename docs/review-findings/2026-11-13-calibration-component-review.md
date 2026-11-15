What this repo does: `components/calibration` saves and loads touch screen calibration data for the CYD display. It keeps one 8-value calibration blob in NVS for each display rotation, applies the saved values on startup, and gives a `calibrate` console command to run touch calibration on the device. Load assumed: single-user embedded firmware on one device; calibration runs rarely and blocks while the user touches the screen.

## Must fix

None.

## Should fix

1. **Command registration can abort startup** (`src/calibration_cmd.c:30`)
   - **What this is:** `calibration_register_commands()` adds the `calibrate` console command during `calibration_init()`.
   - **Problem:** It calls `ESP_ERROR_CHECK(...)` on `esp_console_cmd_register()`. If registration fails, ESP-IDF aborts the program, so device startup can stop inside calibration init instead of reporting an error.
   - **Fix:** Check the return value, log it, and let `calibration_init()` return the error or continue without the command. For example: `esp_err_t err = esp_console_cmd_register(&cmd); if (err != ESP_OK) { ESP_LOGE(...); return err; }`
   - **If we skip it:** A registration problem can reboot or halt the device during boot, even though saved calibration data and the rest of init could still be usable.

2. **Failure paths in `calibration_run()` are not tested** (`src/calibration.c:44-64`, `test/test_calibration.c`)
   - **What this is:** `calibration_run()` does interactive calibration, saves the result to NVS, then applies it to the display.
   - **Problem:** The host tests only cover the success case. The code has separate branches for `display_calibrate_touch()` failure, `calibration_storage_save()` failure, and `display_set_touch_calibration()` failure after save, but there are no tests that prove each branch returns the right error and leaves the expected state.
   - **Fix:** Add mock display hooks that can return errors, then add tests for each failure branch (for example: calibrate fails -> no save/apply; save fails -> error returned; apply fails after save -> saved data remains but function reports failure).
   - **If we skip it:** A future change can break error handling and the tests will still pass, so users may lose a completed calibration or see confusing behavior after a failure.

3. **A storage test claims write-failure coverage but does not check it** (`test/test_calibration_storage.c:46-56`)
   - **What this is:** `test_save_nvs_write_failure_propagates` is named as if it proves NVS write/commit errors are passed back to the caller.
   - **Problem:** The test body does not trigger a blob write or commit failure; it ends with comments explaining the mock cannot target that step. The test still passes, so the test name gives false confidence.
   - **Fix:** Either make the mock able to fail a chosen NVS call and assert the propagated error, or remove/rename this test so it does not claim coverage it does not have.
   - **If we skip it:** Regressions in save error handling can slip through because people think this case is already tested.

## Nice to have

None.

Verdict: Healthy overall; fix command registration abort first, then tighten error-path tests.
Lean: -0 lines, -0 dependencies possible.
Not checked: on-device touch calibration UX and accuracy, real display/NVS timing, integration tests not run in this environment, behavior outside this folder, and callers beyond the references found in the repo.
