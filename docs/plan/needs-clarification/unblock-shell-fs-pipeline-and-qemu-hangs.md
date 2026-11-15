# Unblock shell/FS pipeline and QEMU `shell_fs` hangs

- **ID:** unblock-shell-fs-pipeline-and-qemu-hangs
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/2026-11-14-unblock-shell-fs-pipeline.md`](../2026-11-14-unblock-shell-fs-pipeline.md) (in progress, merge-unblock workaround applied), [`plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md`](../2026-11-15-follow-up-shell-fs-qemu-hangs.md) (open), [`reports/2026-11-12-test-shell-fs-ci-regression-investigation.md`](../../reports/2026-11-12-test-shell-fs-ci-regression-investigation.md), [`reports/2026-11-12-test-shell-regression-new-approach.md`](../../reports/2026-11-12-test-shell-regression-new-approach.md), CI run references in plan docs
- **Approval:** pending

## Summary
`shell_fs` integration failures traced to QEMU hangs around `esp_partition_read` during LittleFS metadata scans in some cases. 11 tests temporarily disabled to unblock merges; pipeline not fully green and root cause/mitigation unresolved. Plan tracks phased work (VFS contracts, seams, evidence-backed fixes).

## Open questions
- [ ] Agreed root-cause hypothesis and mitigation/fix strategy (QEMU flash emulation behavior, esp_littlefs/LittleFS/VFS usage, test environment constraints) with acceptable tradeoffs?
- [ ] Which disabled tests/paths must be restored for closure and what CI evidence is required (stable `shell_fs` across runs)?
- [ ] Scope boundary vs vendor component behavior and vs test-only environment workarounds (without dropping needed platform coverage)?
- [ ] Priority relative to other work and who can run/observe required ESP-IDF v6.1 + QEMU CI iterations?

## Acceptance criteria
- [ ] Root cause/mitigation validated with evidence; disabled tests in `integration_test/test_shell_fs/test_shell_fs.py` restored and TODOs removed; required PR checks (including `shell_fs`) green per plan.

## Validation plan
- Automated: `make format-check`, `make test`; firmware build/lint; integration/QEMU runs (CI or local ESP-IDF v6.1 + QEMU) per `AGENTS.md`.
- Hardware/manual: not primary, but confirm no unintended behavior change on real flash where relevant.

## Progress / evidence
- 2026-11-15: created from in-progress plan + open follow-up (blocked on investigation/CI evidence).
