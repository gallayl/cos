# R22 M3 — `light` command-script migration hardware validation

- **ID:** r22-m3-light-hardware-validation
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/2026-11-12-berry-command-script-migrations.md`](../2026-11-12-berry-command-script-migrations.md) (M3, in progress/validation pending), [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R22), [`reports/2026-11-15-r22-m3-light-hardware-validation.md`](../../reports/2026-11-15-r22-m3-light-hardware-validation.md), code: `components/berry_lang/src/berry_light.cpp`, `storage/{ts,flash_data}/scripts/light.*`
- **Approval:** pending (ready/pre-approved asked after feature specification; clarification decisions recorded 2026-11-20)

## Summary
R22 M3 implementation appears present in code (structured `light.read()` Berry API, `light` scripts, native console registration removed). On-device sensor validation/evidence is pending per plan/report.

## Scope
- Complete and record hardware validation for migrated `light` command per `docs/reports/2026-11-15-r22-m3-light-hardware-validation.md`.
- Update M3/R22 status and evidence in plan docs once validated (or document deviations/fixes needed).

## Non-goals
- M4 `time` read-only view or M5 `ls`/`dir` migrations.
- Unrelated sensor/command changes.

## Decisions (recorded 2026-11-20, clarify-feature, user)
- **Hardware availability/firmware:** **Not available now — explicit deferral** recorded. Unblock conditions: supported CYD hardware with working light sensor, firmware image/commit/build to flash/identify, scripts discoverable per R9/R22 conventions, serial port/device access, and an owner/session to run and record validation (including lighting sweep and error/unavailable evidence or N/A with rationale).
- **Implementation completeness:** **Implementation-complete pending hardware evidence** (Recommended). Treat current implementation (`light.read()` API + scripts, native registration removed) as complete pending only on-device validation/evidence per report checklist; issues found during HW validation become follow-up fixes (scoped/recorded as needed).
- **Evidence ownership/linking:** **User/maintainer performs and records** validation results and updates M3/R22 status/docs; link PR/CI evidence when applicable. Merge/hardware gates remain human actions as applicable.

## Open questions
- None (user decisions recorded 2026-11-20). **Not blocked by unanswered user questions.**

## Blockers / unblock conditions
- **Current blocker (external):** hardware not available now for on-device validation. When available, execute and record the report checklist with firmware/commit/build identity, documented lighting sweep/responsiveness, and error/unavailable behavior (or N/A with rationale), then update M3/R22 status/evidence.

## Acceptance criteria
- [ ] Steps in `docs/reports/2026-11-15-r22-m3-light-hardware-validation.md` executed and recorded (resolution/success path, explicit path, API sanity, error/unavailable behavior as reproducible, regression spot-checks) with firmware/commit/build identity and lighting/error-path evidence (or N/A with rationale).
- [ ] M3/R22 evidence and status updated in `docs/plan/2026-11-12-berry-command-script-migrations.md` and `docs/plan/consolidated-improvement-plan.md` based on results; PR/CI evidence linked when applicable.

## Validation plan
- Automated: `make format-check`, `make test` and scope-appropriate checks if fixes are required (per `AGENTS.md`).
- Hardware/manual: **required** (on-device light sensor behavior) when hardware/session is available.

## Expected areas/files
- `docs/reports/2026-11-15-r22-m3-light-hardware-validation.md`, `docs/plan/2026-11-12-berry-command-script-migrations.md`, `docs/plan/consolidated-improvement-plan.md`, possible fixes under `components/berry_lang/`, `storage/{ts,flash_data}/scripts/`, `frontend/src/cos-sdk/` if issues found.

## Dependencies / prerequisites
- Supported CYD hardware with working light sensor, firmware flashed from the relevant change/commit, scripts discoverable per R9/R22 conventions, serial/device access.

## Risks / constraints
- Cannot be completed without hardware; error/unavailable path may be hard to reproduce (record N/A with evidence).

## Progress / evidence
- 2026-11-15: created; implementation present in repo, hardware validation pending.
- 2026-11-17 (user-provided serial capture, partial; see `docs/reports/2026-11-17-hardware-validation-session-captures.md` R1): `light` command/API/explicit script path (`/flash/scripts/light.be`) executed successfully. Values changed (`0` and `304`) indicating responsiveness, but lighting conditions not documented; repeated reads later returned `0`. Error/unavailable path not reproduced (N/A). Firmware commit/build identity not recorded in capture. **Still incomplete/inconclusive** for full M3 acceptance (needs documented lighting sweep and error/unavailable evidence as reproducible/N/A with rationale).
- 2026-11-18 (prep, no device run): added console-validation inventory and command plans in `docs/reports/2026-11-18-pending-hardware-validation-inventory.md` and `docs/reports/artifacts/hw-pending-2026-11-18/plans/r22-m3-light.txt` (no `/flash` writes). Execution pending serial port/device access and documented lighting/error-path evidence.
- 2026-11-20 (clarify-feature, user): recorded explicit deferral (hardware not available now) with unblock conditions, confirmed implementation-complete pending hardware evidence, and recorded evidence ownership/linking (user/maintainer performs/records; link PR/CI when applicable). Open questions cleared; item remains needs-clarification due to external hardware blocker (not unanswered user questions).
