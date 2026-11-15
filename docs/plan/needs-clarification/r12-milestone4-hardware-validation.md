# R12 — Milestone 4 hardware validation (touch/network)

- **ID:** r12-milestone4-hardware-validation
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R12, P1, pending hardware), [`history/2026-10-improvements.md`](../../history/2026-10-improvements.md) Milestone 4, [`reports/2026-11-08-r9-hardware-validation-guide.md`](../../reports/2026-11-08-r9-hardware-validation-guide.md) (related network validation context)
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
Validate Milestone 4 items on supported CYD hardware: touch scrollback gestures and connected-network `ping`/`nslookup` commands; record results in the source milestone doc and consolidated plan.

## Scope
- Run/record hardware validation for touch scrollback gestures and connected-network `ping`/`nslookup` on supported CYD hardware.
- Capture evidence (device/firmware/commit, steps, pass/fail, logs/screenshots as useful) and update docs.

## Non-goals
- Implementing new features beyond fixes required to satisfy the stated validation (unless separately scoped/approved).
- Unrelated networking/touch changes.

## Acceptance criteria
- [ ] Touch scrollback gestures validated on supported CYD hardware with recorded results.
- [ ] Connected-network `ping`/`nslookup` validated on supported CYD hardware with recorded results (noting current script-resolved command behavior).
- [ ] Results/evidence recorded in `docs/history/2026-10-improvements.md` and `docs/plan/consolidated-improvement-plan.md` (R12).

## Validation plan
- Automated: `make format-check`, `make test` if any doc/code fixes are made (per `AGENTS.md` scope).
- Hardware/manual: **required** (physical device, touch, Wi‑Fi/network) — cannot be completed autonomously without hardware access.

## Expected areas/files
- `docs/history/2026-10-improvements.md`, `docs/plan/consolidated-improvement-plan.md`, possibly `docs/reports/` (new validation report) if fixes/scripts are needed.

## Dependencies / prerequisites
- Access to supported CYD hardware, flashed firmware/build under test, touch-capable setup, Wi‑Fi/network for `ping`/`nslookup`.

## Risks / constraints
- Blocked by hardware availability; command behavior has evolved (e.g. script-resolved `ping`/`nslookup`) — acceptance must be interpreted against current intended behavior.

## Open questions
- [ ] Is supported CYD hardware available now for this validation, and which firmware image/commit should be tested?
- [ ] What are the exact, current acceptance expectations for touch scrollback gestures and for `ping`/`nslookup` (given script migrations/R9/R21/R22 changes) to consider R12 complete?
- [ ] Should a dedicated validation report be created under `docs/reports/`, and who will perform/record the run?

## Progress / evidence
- 2026-11-15: created from pending R12; moved to needs-clarification (hardware/external validation required).
