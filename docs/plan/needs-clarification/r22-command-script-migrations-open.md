# R22 — Berry command-script migrations (open questions / remaining milestones)

- **ID:** r22-command-script-migrations-open
- **Status:** needs-clarification
- **Created:** 2026-11-15
- **Source/links:** [`plan/2026-11-12-berry-command-script-migrations.md`](../2026-11-12-berry-command-script-migrations.md) (M4/M5 not started, open questions, validation checklist items pending), [`plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R22 in progress)
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
R22 has delivered M1/M2a/M2b/M2c and M3 implementation exists (hardware validation tracked separately), but M4 (`time` read-only view), M5 (`ls`/`dir`), and several global open questions/validation decisions remain.

## Scope
- Clarify and, once specified/approved, implement remaining R22 migrations and resolve open questions per the R22 plan (each migration API-gated, contract-preserving or explicitly approved, with tests/evidence).

## Non-goals
- Migrating commands listed as "keep native for now" unless separately justified/approved.
- Using `action()` + console-output parsing for structured results.

## Acceptance criteria
- [ ] Open questions in the R22 plan resolved and recorded (with approved decisions).
- [ ] M4 and/or M5 (as scoped) implemented with structured APIs, scripts, docs, tests, and validation evidence per R22 checklist; native registrations removed only when validated.
- [ ] `docs/plan/2026-11-12-berry-command-script-migrations.md` and consolidated plan R22 updated with status/evidence.

## Validation plan
- Automated: `make format-check`, `make test`, and scope-appropriate checks (firmware build/lint, docs-check, frontend/SDK if affected, integration/QEMU as applicable) per `AGENTS.md`.
- Hardware/manual: likely required for time/NTP context (M4) and flash/SD listing behavior (M5) — to be specified per milestone.

## Expected areas/files
- `components/shell/`, `components/berry_lang/`, `components/time_sync/` or relevant APIs, `components/filesystem/`, `storage/{ts,flash_data}/scripts/`, `frontend/src/cos-sdk/`, `frontend/src/ide/`, `test/`, `integration_test/`, docs/plans/reports.

## Dependencies / prerequisites
- Structured Berry APIs and stable contracts for time/sync-state (M4) and `fs.list`/listing parity (M5); discoverability/bootstrap decisions.

## Risks / constraints
- M5 has high parity/UX risk (flags/format/errors/large dirs/memory); M4 may need hardware/network/NTP context; contract/formatting diffs need explicit approval.

## Open questions
- [ ] Default discoverability: should `/flash/scripts` (or specific dirs) be added to default `path`/bootstrap for stock images?
- [ ] Exact Berry API names/signatures/error schemas for time/sync-state and listing (consistency with existing APIs)?
- [ ] Acceptable output/format differences vs strict parity per command (and is `memory` kept as separate alias/script)?
- [ ] Integration-test/QEMU coverage strategy for time/hardware-dependent or filesystem-parity cases?
- [ ] Staging/scope for M5 (`ls`/`dir` flags/output parity subset) and priority/order now?
- [ ] Which milestone(s), if any, are approved to be specified as ready now?

## Progress / evidence
- 2026-11-15: created to capture R22 open questions and remaining M4/M5 planning needs.
