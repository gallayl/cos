# Vendor `esp_littlefs` component — validation and follow-up

- **ID:** vendor-esp-littlefs-component-validation
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/2026-11-13-vendor-esp-littlefs-component.md`](../2026-11-13-vendor-esp-littlefs-component.md) (implemented, local build/QEMU validation pending ESP-IDF v6.1 env), [`progress/2026-11-13-vendor-esp-littlefs.md`](../../progress/2026-11-13-vendor-esp-littlefs.md)
- **Approval:** pending

## Summary
Vendoring implemented in repo; local/CI validation in an ESP-IDF v6.1 (+ QEMU where relevant) environment and any remaining follow-ups need confirmation/evidence.

## Open questions
- [ ] What validation remains outstanding in the current environment/CI (build, flash image layout, `shell_fs`/filesystem integration given known QEMU hang issues, on-device sanity) and what constitutes “done”?
- [ ] Are there required follow-ups (pin/update policy, config parity, docs) beyond validation?
- [ ] How does this interact with the `shell_fs` QEMU hang investigation/unblock work?

## Acceptance criteria
- [ ] Required validation executed and recorded with evidence; plan/progress docs updated; any identified follow-ups scoped/approved.

## Validation plan
- Automated: `make format-check`, `make test`; `make build`/`make lint` and integration tests in ESP-IDF v6.1/QEMU environment as applicable.
- Hardware/manual: as needed for flash/filesystem sanity.

## Progress / evidence
- 2026-11-15: created (implementation present; validation/follow-up clarification needed).
- 2026-11-19 (cross-link): hardening work (then tracked in `docs/plan/ready/esp-littlefs-stability-hardening.md`, now [`../done/esp-littlefs-stability-hardening.md`](../done/esp-littlefs-stability-hardening.md)) implemented audit-driven hardening in `components/esp_littlefs/` and updated integration tests/README/docs. Local validation evidence recorded there (`make format-check`, `make test`, `make build` pass; `make lint` blocked by pre-existing environment/tooling issues with esp_littlefs excluded from lint SOURCES; `test_esp_littlefs_core` integration build completed, extra build attempted; QEMU runtime/target and SDMMC/BDL/mmap fault-injection gaps remain). This may partially inform validation scope but does not by itself resolve this item's open questions.
- 2026-11-20 (cross-link/finalization): hardening item delivered and moved to done as [`../done/esp-littlefs-stability-hardening.md`](../done/esp-littlefs-stability-hardening.md) (PR [#94](https://github.com/gallayl/cos/pull/94), merge `4ef50c0`, CI run [44558353735](https://github.com/gallayl/cos/actions/runs/44558353735) green including `esp_littlefs_core`/`esp_littlefs_extra` integration tests). Remaining validation/closure questions for vendored esp_littlefs (broader scope, SDMMC/BDL/mmap runtime and hardware fault/error-injection, and any other outstanding environment-specific validation) remain open in this item.
