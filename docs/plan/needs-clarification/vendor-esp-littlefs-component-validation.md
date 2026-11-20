# Vendor `esp_littlefs` component — validation and follow-up

- **ID:** vendor-esp-littlefs-component-validation
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/2026-11-13-vendor-esp-littlefs-component.md`](../2026-11-13-vendor-esp-littlefs-component.md) (implemented, local build/QEMU validation pending ESP-IDF v6.1 env), [`progress/2026-11-13-vendor-esp-littlefs.md`](../../progress/2026-11-13-vendor-esp-littlefs.md)
- **Approval:** pending (no implementation approval recorded; clarification decisions recorded 2026-11-20)

## Summary
Vendoring implemented in repo; hardening delivered (PR [#94](https://github.com/gallayl/cos/pull/94), CI green incl. `esp_littlefs_core`/`esp_littlefs_extra`). Remaining work is to execute/record esp_littlefs-specific validation per agreed scope and document required follow-ups/gaps.

## Decisions (recorded 2026-11-20, clarify-feature, user)
- **Validation scope / definition of done:** **ESP-IDF/QEMU/CI scope + documented gaps, HW sanity opportunistic** (Recommended). Done when esp_littlefs-specific build/integration checks pass in canonical ESP-IDF v6.1 (+QEMU where applicable)/CI and results are recorded; known gaps (SD/MMAP/BDL runtime, hardware fault/error-injection, and any env-specific cases) explicitly documented as follow-ups with unblock conditions. On-device flash/filesystem sanity recorded when hardware available or explicitly deferred with rationale.
- **Follow-ups beyond validation:** **Document pin/update policy + config parity/gaps; no extra refactors** (Recommended). Record/update vendored pin and update policy, document config parity and known gaps/limitations in relevant docs/README/plan items; defer code refactors or broader changes unless validation finds concrete issues.
- **Interaction with `shell_fs` QEMU hang/unblock:** **Related but separable; don’t block on unrelated `shell_fs` hang** (Recommended). Close this item on esp_littlefs-specific validation/evidence. Track `shell_fs` QEMU hang separately (`docs/plan/ready/unblock-shell-fs-pipeline-and-qemu-hangs.md`); only treat it as blocking here if a failure is directly attributable to vendored `esp_littlefs` behavior.

## Open questions
- None (user decisions recorded 2026-11-20). **Not blocked by unanswered user questions.**

## Blockers / unblock conditions
- **Current blockers (external):** execute and record esp_littlefs-specific validation in ESP-IDF v6.1 (+QEMU where applicable)/CI per scope above; document known gaps with unblock conditions. On-device flash/filesystem sanity is opportunistic (record when available or explicitly defer with rationale). `shell_fs` hang work is separate unless a failure is attributable to vendored `esp_littlefs`.

## Acceptance criteria
- [ ] Required esp_littlefs-specific validation executed and recorded with evidence per recorded “done” scope (CI/ESP-IDF v6.1 + QEMU where applicable); known gaps (SD/MMAP/BDL runtime, HW fault/error-injection, env-specific cases) documented with unblock conditions.
- [ ] On-device flash/filesystem sanity recorded when available, or explicitly deferred with rationale.
- [ ] Vendored pin/update policy and config parity/gaps documented (no extra refactors unless validation finds concrete issues); plan/progress docs updated; any identified follow-ups scoped/approved.
- [ ] `shell_fs` QEMU hang treated as separate unless failures are directly attributable to vendored `esp_littlefs` (link evidence/rationale).

## Validation plan
- Automated: `make format-check`, `make test`; `make build`/`make lint` and integration tests in ESP-IDF v6.1/QEMU environment as applicable (per `AGENTS.md`, record exact outcomes/blockers).
- Hardware/manual: opportunistic on-device flash/filesystem sanity (record when available or explicitly defer with rationale).

## Progress / evidence
- 2026-11-15: created (implementation present; validation/follow-up clarification needed).
- 2026-11-19 (cross-link): hardening work (then tracked in `docs/plan/ready/esp-littlefs-stability-hardening.md`, now [`../done/esp-littlefs-stability-hardening.md`](../done/esp-littlefs-stability-hardening.md)) implemented audit-driven hardening in `components/esp_littlefs/` and updated integration tests/README/docs. Local validation evidence recorded there (`make format-check`, `make test`, `make build` pass; `make lint` blocked by pre-existing environment/tooling issues with esp_littlefs excluded from lint SOURCES; `test_esp_littlefs_core` integration build completed, extra build attempted; QEMU runtime/target and SDMMC/BDL/mmap fault-injection gaps remain). This may partially inform validation scope but does not by itself resolve this item's open questions.
- 2026-11-20 (cross-link/finalization): hardening item delivered and moved to done as [`../done/esp-littlefs-stability-hardening.md`](../done/esp-littlefs-stability-hardening.md) (PR [#94](https://github.com/gallayl/cos/pull/94), merge `4ef50c0`, CI run [44558353735](https://github.com/gallayl/cos/actions/runs/44558353735) green including `esp_littlefs_core`/`esp_littlefs_extra` integration tests). Remaining validation/closure questions for vendored esp_littlefs (broader scope, SDMMC/BDL/mmap runtime and hardware fault/error-injection, and any other outstanding environment-specific validation) remain open in this item.
- 2026-11-20 (clarify-feature, user): recorded decisions for validation “done” scope (ESP-IDF/QEMU/CI + documented gaps, HW sanity opportunistic), follow-ups (pin/update policy + config parity/gaps docs, no extra refactors), and separability from `shell_fs` QEMU hang work. Open questions cleared; item remains needs-clarification due to external validation-execution blockers (not unanswered user questions).
