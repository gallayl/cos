# Unblock shell/FS pipeline and QEMU `shell_fs` hangs

- **ID:** unblock-shell-fs-pipeline-and-qemu-hangs
- **Status:** ready (approved)
- **Created:** 2026-11-15
- **Source/links:** [`plan/2026-11-14-unblock-shell-fs-pipeline.md`](../2026-11-14-unblock-shell-fs-pipeline.md) (in progress, merge-unblock workaround applied), [`plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md`](../2026-11-15-follow-up-shell-fs-qemu-hangs.md) (open), [`reports/2026-11-12-test-shell-fs-ci-regression-investigation.md`](../../reports/2026-11-12-test-shell-fs-ci-regression-investigation.md), [`reports/2026-11-12-test-shell-regression-new-approach.md`](../../reports/2026-11-12-test-shell-regression-new-approach.md), [`docs/README.md`](../../README.md), root [`AGENTS.md`](../../../AGENTS.md), CI run references in plan/follow-up docs
- **Approval:** User approved 2026-11-20 (clarify-feature pass, after specification). Pre-approved for implementation/PR/babysit-to-green flow per agreed scope; merge remains human.

## Selection rationale (2026-11-20 clarification pass)

Selected as highest-priority `needs-clarification` item (user request: pick most important): **11 `shell_fs` integration tests are temporarily disabled** to unblock merges (see follow-up/plan), leaving a CI coverage/evidence gap for core shell/FS paths and an unresolved blocker for a fully green `shell_fs` pipeline.

## Summary
`shell_fs` integration failures were traced (by evidence in linked docs) to hangs around `esp_partition_read` during LittleFS metadata scans in QEMU for specific reads (e.g. `part_read start` without matching `part_read done`, `blk=1 off=2304/3072 size=128` in failing cases), after REPL became interactive and other integration suites passed. To unblock merges, 11 tests in `integration_test/test_shell_fs/test_shell_fs.py` were commented out with TODOs. Host/unit coverage for shell FS/VFS behavior was expanded in the unblock effort, but root cause/mitigation is unresolved and the disabled tests remain unrestored.

### Currently disabled tests (from follow-up)
`test_autocomplete_command_and_path`, `test_shell_alias_lifecycle`, `test_berry_filesystem_and_network_apis`, `test_nested_mkdir`, `test_cp_copies_without_overwriting`, `test_cp_rejects_directory_copy`, `test_mv_moves_file`, `test_rmdir_only_removes_empty_directories`, `test_cd_relative`, `test_rm_file`, `test_alias_del`.

## Context reviewed (2026-11-20)
- `docs/plan/2026-11-14-unblock-shell-fs-pipeline.md`: phased unblock work (baseline revert, shell FS command unit tests, VFS contract tests/seam, console/input seam fix to stdin RX resolved class A; class B QEMU flash-read hangs remain), CI evidence across runs (e.g. 43633877205, 43634305900, 43640354444 noted) and 11-test disable workaround to unblock PR #22.
- `docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md`: open follow-up, disabled test list, evidence/hypothesis (`esp_partition_read` hang in QEMU flash emulation), mitigations tried (dir-stat skip, `cmd_ls` direct readdir fast path, yield before read regressed/reverted), suggested investigations (compare `test_shell_fs` vs `vfs_ops`/`esp_littlefs`, narrow config knobs, evaluate newer QEMU override, or scoped workaround with documented rationale/compensating coverage and explicit approval).
- `docs/reports/2026-11-12-test-shell-fs-ci-regression-investigation.md` and `docs/reports/2026-11-12-test-shell-regression-new-approach.md`: earlier regression analysis/repro notes (mount-fail `-84`/format-remount flow, `fopen("w")`/LittleFS stalls under some conditions, local Windows/QEMU partial repro, bisect/isolation suggestions). Historical context; may not fully match final "part_read start/no done" evidence but relevant to trigger conditions.

## Decisions (recorded, 2026-11-20 — user, clarify-feature pass)
- **Outcome scope:** **Restore all 11 tests**. Re-enable every currently disabled `shell_fs` test in `integration_test/test_shell_fs/test_shell_fs.py` and make them pass reliably in the current CI environment (ESP-IDF v6.1 + bundled QEMU) via evidence-backed fix or mitigation, with **no reduction in required integration/platform coverage**.
- **Strategy / change boundaries:** **Cross-boundary, minimal evidence-backed changes allowed**. Prefer fix/mitigation in shell/VFS/FS/test app as needed; minimal evidence-backed changes are also allowed in vendored `components/esp_littlefs` and in the CI/integration environment (e.g. QEMU tool/version override or test config) when justified by evidence.
- **Closure evidence:** **One green CI run** is sufficient, provided `shell_fs` and all other required integration jobs/checks pass on the relevant PR/branch (CI authoritative).
- **Validation/debug environment:** **CI iterations available; no local ESP-IDF v6.1 + QEMU** debugging environment assumed. Changes to be validated via CI runs on a branch/PR; do not claim unrun local build/lint/integration checks (report exact CI/local outcomes per `AGENTS.md`).

## Scope
- **In scope:** investigate and implement an evidence-backed root-cause fix or reliable mitigation for the QEMU `shell_fs` hangs (documented around LittleFS metadata scans / `esp_partition_read` `part_read start` without `part_read done` in affected cases), so that all 11 currently disabled tests can be restored and pass reliably in CI; update affected code/tests/docs/evidence accordingly. Minimal, evidence-backed changes may include `integration_test/test_shell_fs/`, shell/VFS/filesystem code, vendored `components/esp_littlefs`, and/or CI/integration environment configuration (e.g. QEMU/tooling) as needed.
- **Non-goals:** unrelated shell/FS feature work; broad refactors or vendored LittleFS rewrites without evidence; reducing required platform coverage; speculative changes without CI/evidence feedback.

## Open questions
- **None.** The concrete technical fix/mitigation is to be determined by evidence-backed investigation/iteration (CI-authoritative) within the agreed scope and acceptance criteria.

## Acceptance criteria / definition of done
- [ ] Evidence-backed root cause and/or mitigation is identified and implemented (documented with relevant CI run IDs/log evidence and rationale for any environment/tooling change).
- [ ] All 11 currently disabled tests in `integration_test/test_shell_fs/test_shell_fs.py` are restored (uncommented/enabled), TODO/workaround markers for this hang are removed, and tests pass reliably in CI (no command-time hangs/timeouts).
- [ ] A CI run on the relevant PR/branch shows `shell_fs` and all other required integration jobs/checks passing (one green CI run sufficient per decision); no unintended regressions in required suites.
- [ ] Relevant plan/follow-up/evidence docs are updated to reflect resolution and closure evidence (at minimum [`plan/2026-11-14-unblock-shell-fs-pipeline.md`](../2026-11-14-unblock-shell-fs-pipeline.md) and/or [`plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md`](../2026-11-15-follow-up-shell-fs-qemu-hangs.md), and this queue item’s status/links per docs state model when delivered). If behavior on real flash could be affected by firmware/VFS/storage behavior changes, note hardware/manual verification status explicitly.

## Validation plan
- **Automated (local, when feasible):** `make format-check`, `make test` (host). If C/C++/build-affecting changes are made, run `make build` and/or `make lint` when the ESP-IDF v6.1 environment is available; otherwise record as not run with actionable blocker (per `AGENTS.md`).
- **CI-authoritative integration:** iterate via CI (ESP-IDF v6.1 + QEMU) until `shell_fs` and other required integration suites pass; capture run/job IDs and outcomes as evidence. No local ESP-IDF v6.1 + QEMU environment assumed.
- **Host/unit coverage:** keep or extend deterministic coverage (e.g. shell FS command semantics, VFS contracts/seams) where appropriate per rebalance rules; do not replace required integration/platform coverage.
- **Hardware/manual:** not primary for this QEMU hang, but if firmware/VFS/storage behavior changes, record whether real-flash behavior was verified or remains unverified/pending.

## Expected areas/files (rough)
- `integration_test/test_shell_fs/test_shell_fs.py` (restore disabled tests/remove TODOs), `integration_test/test_shell_fs/main/*` (if test app changes needed)
- `components/shell/src/*`, `components/filesystem/src/*`, `components/esp_littlefs/*` (only as evidence requires)
- `.github/workflows/integration-test.yml` and/or integration/QEMU tooling configuration (only if evidence-backed environment mitigation needed)
- `docs/plan/2026-11-14-unblock-shell-fs-pipeline.md`, `docs/plan/2026-11-15-follow-up-shell-fs-qemu-hangs.md`, `docs/reports/*` (evidence updates), this queue item / consolidated plan links as applicable

## Dependencies / prerequisites
- Access to GitHub Actions/CI iterations on a branch/PR for authoritative ESP-IDF v6.1 + QEMU integration validation (stated available).
- Linked evidence and prior investigation artifacts (CI run logs, disabled test list, instrumentation findings in plan/follow-up/reports).

## Risks / constraints
- Root cause may be QEMU/emulation- or timing-specific; evidence-driven iteration may require multiple CI runs despite one-green-run closure threshold.
- No local ESP-IDF v6.1 + QEMU debugging assumed, so turnaround depends on CI feedback.
- Vendored `components/esp_littlefs` or CI environment changes carry maintenance/compatibility risk and must be minimal, justified, and documented.
- Must not weaken required platform coverage; host/unit tests complement but do not replace required integration coverage.

## Progress / evidence
- 2026-11-15: created from in-progress plan + open follow-up (blocked on investigation/CI evidence).
- 2026-11-20 (clarify-feature pass, round 1): selected as highest-priority needs-clarification item (CI coverage blocker: 11 disabled `shell_fs` tests). Reviewed linked plan/follow-up/reports and current item state; refined context, draft scope/non-goals/acceptance/validation/dependencies/risks, and open questions. User decisions recorded (restore all 11 tests; cross-boundary minimal evidence-backed changes allowed including vendored esp_littlefs and CI/QEMU env if justified; one green CI run sufficient with all required integration jobs/checks passing; CI iterations available, no local ESP-IDF+QEMU). Spec gaps resolved; open questions: none.
- 2026-11-20 (clarify-feature pass, approval): **explicit user approval** recorded to mark ready and move to `docs/plan/ready/` (pre-approved for implementation/PR flow; merge remains human). Item promoted to **ready** at `docs/plan/ready/unblock-shell-fs-pipeline-and-qemu-hangs.md`.