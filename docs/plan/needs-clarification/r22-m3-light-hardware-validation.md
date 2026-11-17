# R22 M3 — `light` command-script migration hardware validation

- **ID:** r22-m3-light-hardware-validation
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/2026-11-12-berry-command-script-migrations.md`](../2026-11-12-berry-command-script-migrations.md) (M3, in progress/validation pending), [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R22), [`reports/2026-11-15-r22-m3-light-hardware-validation.md`](../../reports/2026-11-15-r22-m3-light-hardware-validation.md), code: `components/berry_lang/src/berry_light.cpp`, `storage/{ts,flash_data}/scripts/light.*`
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
R22 M3 implementation appears present in code (structured `light.read()` Berry API, `light` scripts, native console registration removed). On-device sensor validation/evidence is pending per plan/report.

## Scope
- Complete and record hardware validation for migrated `light` command per `docs/reports/2026-11-15-r22-m3-light-hardware-validation.md`.
- Update M3/R22 status and evidence in plan docs once validated (or document deviations/fixes needed).

## Non-goals
- M4 `time` read-only view or M5 `ls`/`dir` migrations.
- Unrelated sensor/command changes.

## Acceptance criteria
- [ ] Steps in `docs/reports/2026-11-15-r22-m3-light-hardware-validation.md` executed and recorded (resolution/success path, explicit path, API sanity, error/unavailable behavior as reproducible, regression spot-checks).
- [ ] M3/R22 evidence and status updated in `docs/plan/2026-11-12-berry-command-script-migrations.md` and `docs/plan/consolidated-improvement-plan.md` based on results.

## Validation plan
- Automated: `make format-check`, `make test` and scope-appropriate checks if fixes are required (per `AGENTS.md`).
- Hardware/manual: **required** (on-device light sensor behavior).

## Expected areas/files
- `docs/reports/2026-11-15-r22-m3-light-hardware-validation.md`, `docs/plan/2026-11-12-berry-command-script-migrations.md`, `docs/plan/consolidated-improvement-plan.md`, possible fixes under `components/berry_lang/`, `storage/{ts,flash_data}/scripts/`, `frontend/src/cos-sdk/` if issues found.

## Dependencies / prerequisites
- Supported CYD hardware with working light sensor, firmware flashed from the relevant change/commit, scripts discoverable per R9/R22 conventions.

## Risks / constraints
- Cannot be completed without hardware; error/unavailable path may be hard to reproduce (record N/A with evidence).

## Open questions
- [ ] Is hardware available to run the validation now, and on which firmware/commit should it be performed?
- [ ] Is M3 considered implementation-complete pending only hardware evidence, or are there known code/contract gaps to address first?
- [ ] Who records results and updates M3/R22 status (and should PR/CI evidence be linked once applicable)?

## Progress / evidence
- 2026-11-15: created; implementation present in repo, hardware validation pending.
- 2026-11-17 (user-provided serial capture, partial; see `docs/reports/2026-11-17-hardware-validation-session-captures.md` R1): `light` command/API/explicit script path (`/flash/scripts/light.be`) executed successfully. Values changed (`0` and `304`) indicating responsiveness, but lighting conditions not documented; repeated reads later returned `0`. Error/unavailable path not reproduced (N/A). Firmware commit/build identity not recorded in capture. **Still incomplete/inconclusive** for full M3 acceptance (needs documented lighting sweep and error/unavailable evidence as reproducible/N/A with rationale).
