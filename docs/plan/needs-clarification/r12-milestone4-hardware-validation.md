# R12 — Milestone 4 hardware validation (touch/network)

- **ID:** r12-milestone4-hardware-validation
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R12, P1, pending hardware), [`history/2026-10-improvements.md`](../../history/2026-10-improvements.md) Milestone 4, [`reports/2026-11-08-r9-hardware-validation-guide.md`](../../reports/2026-11-08-r9-hardware-validation-guide.md) (related network validation context)
- **Approval:** pending (ready/pre-approved asked after feature specification; clarification decisions recorded 2026-11-20)

## Summary
Validate Milestone 4 items on supported CYD hardware: touch scrollback gestures and connected-network `ping`/`nslookup` commands; record results in the source milestone doc and consolidated plan.

## Scope
- Run/record hardware validation for touch scrollback gestures and connected-network `ping`/`nslookup` on supported CYD hardware.
- Capture evidence (device/firmware/commit, steps, pass/fail, logs/screenshots as useful) and update docs.

## Non-goals
- Implementing new features beyond fixes required to satisfy the stated validation (unless separately scoped/approved).
- Unrelated networking/touch changes.

## Decisions (recorded 2026-11-20, clarify-feature, user)
- **Hardware availability/firmware:** **Not available now — explicit deferral** recorded. Unblock conditions: supported CYD hardware with touch and Wi‑Fi/network access, firmware image/commit/build to test, and who performs/records the validation run.
- **Acceptance expectations:** **Validate against current intended behavior** (Recommended). Touch scrollback gestures must work as currently implemented on supported CYD hardware. `ping`/`nslookup` must behave correctly per current script-resolved command contracts on a connected network (resolution/execution, usage/errors/success as applicable), with evidence recorded against current behavior (not legacy native-only assumptions).
- **Evidence/report ownership:** **Yes, create a dedicated validation report in `docs/reports/`**; user/maintainer with hardware performs and records. Record results/evidence there and update `docs/history/2026-10-improvements.md` and `docs/plan/consolidated-improvement-plan.md` (R12) with links; link PR/CI if fixes/docs updates land via PR.

## Open questions
- None (user decisions recorded 2026-11-20). **Not blocked by unanswered user questions.**

## Blockers / unblock conditions
- **Current blocker (external):** hardware not available now for on-device validation. When available, execute validation on supported CYD with identified firmware/commit/build and connected network, create/record a dedicated report in `docs/reports/`, and update milestone/plan docs with evidence.

## Acceptance criteria
- [ ] Touch scrollback gestures validated on supported CYD hardware with recorded results, interpreted against **current intended behavior**.
- [ ] Connected-network `ping`/`nslookup` validated on supported CYD hardware with recorded results per **current script-resolved command contracts** (not legacy native-only assumptions).
- [ ] Dedicated validation report created in `docs/reports/` and results/evidence recorded/linked in `docs/history/2026-10-improvements.md` and `docs/plan/consolidated-improvement-plan.md` (R12). PR/CI linked if applicable.

## Validation plan
- Automated: `make format-check`, `make test` if any doc/code fixes are made (per `AGENTS.md` scope).
- Hardware/manual: **required** when available (physical device, touch, Wi‑Fi/network) — cannot be completed autonomously without hardware access.

## Expected areas/files
- `docs/reports/` (dedicated R12 validation report), `docs/history/2026-10-improvements.md`, `docs/plan/consolidated-improvement-plan.md`, this item; possible fixes if needed (separately scoped).

## Dependencies / prerequisites
- Access to supported CYD hardware, flashed firmware/build under test (commit recorded), touch-capable setup, Wi‑Fi/network for `ping`/`nslookup`.

## Risks / constraints
- Blocked by hardware availability; command behavior has evolved (e.g. script-resolved `ping`/`nslookup`) — acceptance interpreted against current intended behavior as recorded above.

## Progress / evidence
- 2026-11-15: created from pending R12; moved to needs-clarification (hardware/external validation required).
- 2026-11-17 (user-provided serial captures, partial data; see `docs/reports/2026-11-17-hardware-validation-session-captures.md` Sections A/D, R4): on connected network, `ping 8.8.8.8` (36 ms, 42 ms), `ping 127.0.0.1` (1 ms), and `nslookup example.com` (`172.66.147.243`) succeeded in captured runs. Touch scrollback gesture validation and full recorded pass/fail evidence remain pending. Firmware commit/build identity not consistently recorded.
- 2026-11-18 (prep, no device run): added console-validation inventory and command plans in `docs/reports/2026-11-18-pending-hardware-validation-inventory.md` and `docs/reports/artifacts/hw-pending-2026-11-18/plans/` (no `/flash` writes). Execution pending serial port/device access and manual touch validation.
- 2026-11-20 (clarify-feature, user): recorded explicit deferral (hardware not available now) with unblock conditions, acceptance interpreted against current intended/script-resolved behavior, and decision to create a dedicated `docs/reports/` validation report (user/maintainer performs/records). Open questions cleared; item remains needs-clarification due to external hardware blocker (not unanswered user questions).
