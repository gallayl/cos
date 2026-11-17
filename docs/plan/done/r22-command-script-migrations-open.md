# R22 — Berry command-script migrations (open questions / remaining milestones)

- **ID:** r22-command-script-migrations-open
- **Status:** done (clarification resolved for M5; moved to ready)
- **Created:** 2026-11-15
- **Updated:** 2026-11-17 (clarify-feature: M5 unblocked and moved to ready)
- **Source/links:** [`../2026-11-12-berry-command-script-migrations.md`](../2026-11-12-berry-command-script-migrations.md) (R22 details), [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R22), **M4 (delivered):** [`../done/r22-m4-time-read-only.md`](./r22-m4-time-read-only.md), **M5 (ready):** [`../ready/r22-m5-ls-dir-command-scripts.md`](../ready/r22-m5-ls-dir-command-scripts.md), **M3 (separate, needs clarification):** [`../needs-clarification/r22-m3-light-hardware-validation.md`](../needs-clarification/r22-m3-light-hardware-validation.md)
- **Approval:** M4 explicitly approved and pre-approved for implementation/PR flow on 2026-11-15 (user chat; merge remains human). M4 delivered/moved to done (PR #39, required CI green; on-device spot-check recorded as user-confirmed 2026-11-16, human merge pending per M4 done item). **M5 spec explicitly approved in chat on 2026-11-15** (draft accepted as final, no corrections 2026-11-17). **2026-11-17 (user chat):** accepted existing M4 on-device spot-check record as satisfying M5 gate and explicitly approved moving M5 to ready now; M5 implementation/PR-flow pre-approved (merge remains human). M5 moved to `docs/plan/ready/r22-m5-ls-dir-command-scripts.md`.

## Summary
R22 has delivered M1/M2a/M2b/M2c. M3 implementation exists with hardware validation tracked separately in [`../needs-clarification/r22-m3-light-hardware-validation.md`](../needs-clarification/r22-m3-light-hardware-validation.md). M4 (`time` read-only view) is delivered/moved to done (PR #39, required CI green; on-device spot-check user-confirmed 2026-11-16, human merge pending per M4 done item). **M5 (`ls`/`dir`) clarification is resolved:** spec final/approved, blocker cleared (M4 evidence accepted 2026-11-17), and M5 moved to [`../ready/r22-m5-ls-dir-command-scripts.md`](../ready/r22-m5-ls-dir-command-scripts.md) (pre-approved for implementation/PR flow; merge remains human).

## Scope
- Historical tracker for R22 clarification decisions and M4/M5 specification. M5 is now specified as ready in its own ready item; M3 remains tracked separately in needs-clarification.

## Non-goals
- Migrating commands listed as "keep native for now" unless separately justified/approved.
- Using `action()` + console-output parsing for structured results.
- Moving clock synchronization into scripts (M4 sync control remains native).

## Decisions / resolved questions (2026-11-15, 2026-11-17)

- [x] **Which milestone(s) to specify ready now?** **M4 only** in the 2026-11-15 cycle. M4 moved to ready (2026-11-15) and later delivered to done; M5 clarified, unblocked 2026-11-17, and moved to ready.
- [x] **Default discoverability / bootstrap:** Add **`/flash/scripts` as a built-in default search directory** in the script resolver so stock migrated command scripts work out-of-box even with an empty `/flash/.cos_path`. Resolver behavior documented (implicit default, searched per resolver rules; user `path` config still applies). Stock image/script placement remains under `/flash/scripts`.
- [x] **`memory` alias/script:** **Keep both `mem` and `memory` as separate scripts** (`mem.be`/`memory.be`, and TS sources where maintained), matching current migrated state. Intentional, no consolidation required.
- [x] **M4 Berry API (exact shape):** Add **`system.time_info()`** (Berry module `system`, TS SDK `@cos/system` / global `system`) returning a structured map (no console scraping): Success `{ ok: true, synced: boolean, epoch: number, utc: string, local: string }` backed by `time_sync_is_synced/get_epoch/get_utc/get_local`; Failure `{ ok: false, error: string }` (ESP-IDF/stable error name).
- [x] **M4 command contract/parity:** **Strict parity** with native `time` (`time`/unknown args -> full status `Local/UTC/Synced`; `time utc`; `time epoch`; format error -> `time: format error (<error>)`, exit 1). Sync remains native.
- [x] **M4 validation:** **Host/automated + on-device spot-check.** Automated per `AGENTS.md` scope; on-device spot-check recorded as user-confirmed 2026-11-16 (detailed log not in-repo). QEMU not required for NTP.
- [x] **M5 staging/scope/priority:** M5 spec approved 2026-11-15 (final, no corrections 2026-11-17). **2026-11-17:** user accepted M4 on-device spot-check record as satisfying gate; M5 moved to `docs/plan/ready/r22-m5-ls-dir-command-scripts.md` with implementation/PR-flow pre-approval (merge remains human).
- [x] **M5 spec (final):** real file sizes (intentional diff from native readdir fast-path 0-size quirk), flags `-a`/`-l`/`-la`/`-al`, deterministic name-sorted output, defined root/errors/usage/exit behavior, best-effort truncation warning when `fs.list` count==64 (exit 0), `ls`/`dir` identical behavior. `fs.list` expected sufficient; confirm during implementation (re-clarify if additive changes needed). See ready item for full contract.
- [x] **M5 unblock gate (2026-11-17):** satisfied by accepted M4 on-device spot-check record (human merge of M4 may remain pending separately).
- [x] **M5 pre-approval (2026-11-17):** implementation/PR flow pre-approved (merge remains human).
- [x] **Integration-test/QEMU strategy:** **Host tests + targeted QEMU where feasible.** No QEMU required for NTP/time sync; on-device validation for time/network-dependent cases.

## Acceptance criteria
- [x] Open questions relevant to **M4 ready specification** resolved and recorded. M4 delivered to done.
- [x] **M4** specified, approved, implemented, and delivered to `docs/plan/done/r22-m4-time-read-only.md` (PR #39, required CI green). On-device spot-check recorded as user-confirmed (2026-11-16; detailed log not in-repo); human merge remains pending per M4 done item.
- [x] **M5** fully specified and explicitly approved as final. Ready-contract fields satisfied and moved to `docs/plan/ready/r22-m5-ls-dir-command-scripts.md` on 2026-11-17 after user accepted M4 evidence and approved move to ready (implementation/PR-flow pre-approved; merge remains human).
- [x] M4 implemented with structured API, script(s), docs, tests, and validation evidence per R22 checklist (see M4 done item).
- [x] M5 clarification blocker resolved and M5 moved to `docs/plan/ready/` (implementation/validation tracked in ready item).
- [ ] `docs/plan/2026-11-12-berry-command-script-migrations.md` and consolidated plan R22 updated with status/evidence as work progresses (follow-up).

## Open questions / blockers
- [x] **M5 dependency blocker (resolved 2026-11-17):** user accepted existing M4 on-device spot-check record (user-confirmed 2026-11-16 in `docs/plan/done/r22-m4-time-read-only.md`) as satisfying the recorded gate. M5 moved to `docs/plan/ready/r22-m5-ls-dir-command-scripts.md` with implementation/PR-flow pre-approval effective (merge remains human). Human merge of M4 may remain pending separately.
- [ ] **M5 implementation-time check (prerequisite, tracked in ready item):** confirm current `fs.list` API contract is sufficient as-is for the approved spec during implementation; if additive fields/behavior changes are needed, re-clarify before deviating.
- [ ] **M3 (separate item):** hardware validation evidence/blockers tracked in [`../needs-clarification/r22-m3-light-hardware-validation.md`](../needs-clarification/r22-m3-light-hardware-validation.md). No M5 spec questions remain open in this tracker.

## Progress / evidence
- 2026-11-15: created to capture R22 open questions and remaining M4/M5 planning needs.
- 2026-11-15 (clarification): user decisions recorded — M4 only to specify ready now; add `/flash/scripts` to built-in default resolver search; keep `mem` and `memory` as separate scripts; M4 API `system.time_info()` structured map; strict parity for `time`; sync native; host/automated + on-device spot-check; M5 deferred; host tests + targeted QEMU where feasible.
- 2026-11-15 (approval): M4 spec approved and pre-approved for implementation/PR flow; M4 moved to ready (historical).
- 2026-11-15 (clarify-feature, rounds 1-3): M5 spec clarified (real sizes, `-a/-l/-la/-al`, sorting/root/errors, truncation warning count==64 exit 0), drafted, and explicitly approved final; kept in needs-clarification pending M4 validation-evidence gate.
- 2026-11-17 (clarify-feature): reconciled M4 state/links (M4 done, PR #39, CI green, on-device spot-check user-confirmed 2026-11-16, merge pending). Recorded M5 gate and conditional implementation/PR-flow pre-approval.
- 2026-11-17 (clarify-feature, follow-up review): re-reviewed target/M4-done/M3/R22 source plan and code state. No new M5 spec gaps found. User chose **"Accept evidence, move M5 to ready now"**. Treated M4 evidence as satisfying gate, resolved M5 blocker, created `docs/plan/ready/r22-m5-ls-dir-command-scripts.md` (Ready contract satisfied, pre-approved for implementation/PR flow, merge remains human), and moved this tracker from `docs/plan/needs-clarification/` to `docs/plan/done/`. No implementation or validation run.
- 2026-11-17 (user-provided serial captures): detailed in-repo R2 capture recorded for M4 `time` parity/API in synced state (`docs/reports/2026-11-17-hardware-validation-session-captures.md` R2), reinforcing accepted gate evidence (commit/build identity still not recorded). R1 capture for M3 `light` remains partial/inconclusive and tracked in `docs/plan/needs-clarification/r22-m3-light-hardware-validation.md`.