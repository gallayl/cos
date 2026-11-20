# DX: Delivery acceleration from recent CI/validation bottlenecks

- **ID:** dx-delivery-acceleration-bottlenecks
- **Status:** needs-clarification (umbrella triage; v1 first slice implemented/delivered and moved to done — PR [#103](https://github.com/gallayl/cos/pull/103), CI green, pending human review/approval/merge)
- **Created:** 2026-11-20
- **Source/links:**
  - Evidence (this draft is derived from): `git log --since="5 days ago"` / PRs `fb458ad`…`5ec95d3` (2026-11-17→2026-11-20), `.github/workflows/ci.yaml` and reusable workflows, `docs/plan/consolidated-improvement-plan.md`, `docs/plan/ready/` (5 items at time of draft), `docs/plan/needs-clarification/` (17 items at time of draft)
  - Reports: [`docs/reports/2026-11-11-dx1-baseline-measurements.md`](../../reports/2026-11-11-dx1-baseline-measurements.md), [`docs/reports/2026-11-18-pending-hardware-validation-inventory.md`](../../reports/2026-11-18-pending-hardware-validation-inventory.md), [`docs/reports/2026-11-19-hw-validation-only-missing-com3.md`](../../reports/2026-11-19-hw-validation-only-missing-com3.md)
  - Related: [`docs/plan/needs-clarification/developer-tooling-dx-improvement-plan.md`](./developer-tooling-dx-improvement-plan.md), [`docs/plan/done/dx1-canonical-baseline-report.md`](../done/dx1-canonical-baseline-report.md), [`docs/plan/needs-clarification/unblock-shell-fs-pipeline-and-qemu-hangs.md`](./unblock-shell-fs-pipeline-and-qemu-hangs.md), pending hardware-validation items listed in the 2026-11-18 inventory
  - First v1 slice (done, delivered; pending human review/approval/merge): [`dx-hw-preflight-and-delivery-evidence-templates`](../done/dx-hw-preflight-and-delivery-evidence-templates.md) (PR [#103](https://github.com/gallayl/cos/pull/103), CI run [44722447791](https://github.com/gallayl/cos/actions/runs/44722447791) success)
- **Approval:** pending for this umbrella. First slice was explicitly approved (2026-11-20) and implemented/delivered in PR #103 (merge remains a human/maintainer action).

## Summary

Umbrella triage item for an evidence-backed improvement program to reduce cycle time from implementation to merge-ready/delivered features. Observed focus areas: hardware/manual validation tail and setup fragility, local↔CI parity and expensive checks, integration/QEMU cost and flakiness, and plan/PR/finalization overhead. This file captures evidence, candidate slices, and triage decisions; it does **not** implement tooling/CI/runtime changes. Per 2026-11-20 decisions, v1 prioritizes **hardware validation lane + process/merge-readiness streamlining** and will be delivered via a **separate, fully specified ready item** (this umbrella remains in `needs-clarification` as the program/triage record, similar to `developer-tooling-dx-improvement-plan`).

## Decisions (recorded)

- **2026-11-20 (user, clarify-feature round 1): v1 priority/scope:** **HW validation + process streamlining** — start with hardware-validation preflight/batching/evidence workflow and merge-readiness/PR-plan/queue streamlining (targets observed HW tail/setup drift and finalization/queue churn; low CI-signal risk).
- **2026-11-20 (user, clarify-feature round 1): Item shape:** **Split: umbrella triage + separate first ready slice** — keep this file in `needs-clarification` as umbrella/program record; create/specify a separate ready item for the chosen v1 slice once fully specified/approved (DX1 precedent).
- **2026-11-20 (user, clarify-feature round 1): Session goal:** **Drive first chosen slice to Ready now** — continue specification (scope/non-goals, acceptance/DoD, validation, metrics/baseline, ownership/cadence, dependencies/risks, expected areas/files) and request/record explicit approval after specification before moving the first slice item to `plan/ready/`.

## Observed bottlenecks (evidence snapshot, 2026-11-17→2026-11-20)

| Area | Evidence | Delivery impact |
|---|---|---|
| Hardware/manual validation tail and fragility | Inventory/report show many items pending on-device/manual evidence or sign-off; prepared plans exist but sessions depend on device/env. 2026-11-19 COM3 run recorded inconclusive captures/recovery reset, heap-pressure `NO_MEM`/timeout in network checks, SD script/runtime drift (`/sdcard/scripts/ls.be: 'string' has no method 'charAt'`), `autoexec.be` entrypoint mismatch, and low-memory diagnostics caveats on affected builds. `pyserial` missing in active Python env blocked automation until installed. | Code/CI can be green while items remain pending human validation/closure; device time spent on setup/debug instead of evidence. |
| Integration/QEMU cost and instability | DX1 CI (run 44332595802, 8 suites) ~1h25m wall with long/late jobs (e.g. `test_shell_fs` ~9m14s). Recent stabilization/fix commits: `b7a3616`, `67bd223`; QEMU gaps/hangs tracked (`unblock-shell-fs-pipeline-and-qemu-hangs.md`, skipped/harness limitations noted in reports/plans). Some behavior remains hardware-only (upload stress/QEMU gaps, CYD/BT/peripheral/touch). | Long/variable feedback, reruns, and non-automatable gaps delay confidence and merge-readiness. |
| Local↔CI parity and expensive build/lint | DX1: local `make lint` failed in Windows env (74.3s, exit 2, severe clang-tidy/header/compile-DB alignment issues) while CI `espressif/idf:v6.1` passed; `idf.py`/QEMU often unavailable locally. Clean `make build` 139.5s vs warm ccache 72.1s (~1.94×). | Repro friction, CI-as-discovery, slower local iteration. |
| Process/finalization and queue churn | Commit patterns include feature→harden/fix follow-ups, CI-flow repair sequences, and docs-only finalize/move-to-done/evidence PRs; `ready` queue small (5) vs `needs-clarification` large (17) at draft time. | Admin/context-switch overhead per feature and slower pull-through from “implemented” to “delivered/closed”. |
| Coupled changes and memory sensitivity | Berry/SDK/native bindings/compiled artifacts/inventory must stay in sync for script-facing changes; memory/lifecycle/diagnostics work recurs across plans and can surface only under pressure/hardware. | Higher validation matrix and debug cost for cross-cutting/runtime changes. |

## Candidate improvement slices (draft options)

1. **Hardware validation lane (batching + preflight + evidence)**
   - Preflight: `pyserial`, port, flashed firmware identity/ELF/profile, SD/mount state, reboot allowed, quick read-only probe (`boot_profile`, `services`, `mem`, `svc_mem`, `wifi status`, `path list`) and known-blocker checks.
   - Execution: batch read-only/default-profile checks per session, then per reboot/profile matrix; reuse inventory command plans; standard log/outcome/anomaly template.
   - Hygiene: detect/remediate device-state drift (script/runtime mismatch, `autoexec` issues) and isolate known build blockers before broad validation.
   - Cadence/ownership: define regular batched validation pass and track “CI green but HW pending” items.

2. **Local↔CI parity and fast feedback**
   - Blessed environment/docs (devcontainer or documented ESP-IDF v6.1 + toolchain), preflight for `idf.py`/clang-tidy/QEMU/compile DB freshness.
   - Standardize/localize known wins where safe (e.g. ccache) and document expected local “confidence set” (format, host tests, scoped frontend/lint/build).
   - Keep CI as confirmation by making common failures reproducible locally.

3. **Integration/QEMU throughput and determinism**
   - Use DX1 timings/failure history to tier suites (required PR smoke vs extended/nightly/quarantine-with-owner) and reduce wall-clock/flake impact.
   - Continue determinism fixes (explicit readiness, bounded retries only where justified), document QEMU-unsupported cases, and prefer host tests for logic where feasible.

4. **Merge-readiness and documentation/process streamlining**
   - Standard PR/plan evidence templates (exact local checks, CI run IDs, HW gaps, remaining human actions) to reduce follow-up “record evidence/finalize” churn.
   - Where policy allows, combine implementation + plan-state/cross-link updates in the same PR once criteria are met; keep `ready`/`needs-clarification` queues accurate with minimal overhead.
   - Triage `needs-clarification` backlog (promote high-value, split, defer/archive low-ROI) to maintain pickable flow.

5. **Coupled-change guardrails (Berry/SDK/runtime/memory)**
   - Reinforce sync/checklist for script-facing API changes (declarations, native bindings, inventory/manifest/tests, compiled examples) and include device-script compatibility in HW checklist when relevant.
   - Standard cheap A-B evidence (`mem`/`berry profile`/`svc_mem`) for lifecycle/memory changes and prioritize fixes that unblock broad validation.

## Non-goals (for this umbrella triage item)

- Implementing CI workflow changes, scripts, devcontainer/tooling, firmware/frontend/runtime behavior, or dependency upgrades in this umbrella item.
- Declaring improvements approved, scheduled, or complete without explicit recorded approval and evidence.
- Claiming checks or hardware validation were executed beyond evidence cited from existing reports/history.
- Moving this umbrella to `plan/ready/` as a whole program; ready work must be tracked as separate, fully specified slice item(s) with explicit approval after specification.

## Acceptance criteria (for this umbrella triage item)

- [x] Prioritized v1 direction selected and recorded: **HW validation + process streamlining** (2026-11-20, user).
- [x] Delivery shape selected and recorded: **split** (umbrella stays needs-clarification; first v1 slice specified as a separate ready item) (2026-11-20, user).
- [x] First v1 slice specified and explicitly approved (Ready-contract fields complete, **Open questions: none**), moved to `docs/plan/ready/`, then implemented and delivered: [`dx-hw-preflight-and-delivery-evidence-templates`](../done/dx-hw-preflight-and-delivery-evidence-templates.md) (PR [#103](https://github.com/gallayl/cos/pull/103), CI run [44722447791](https://github.com/gallayl/cos/actions/runs/44722447791) success, moved to `docs/plan/done/`; pending human review/approval/merge). Cross-links updated in umbrella/slice/tracker as applicable.
- [ ] Remaining/deferred slices (local↔CI parity, integration/QEMU throughput/tiering, coupled-change guardrails, and any out-of-scope HW/process sub-areas) kept explicit here with rationale and what would make them promotable.

## Validation plan (for this clarification/specification pass)

- **This pass (docs/planning):** review and update markdown only. No runtime/build validation required by scope; when docs-only changes are committed, run `make format-check`, `make test`, and `make docs-check` and report exact outcomes per `AGENTS.md`.
- **First v1 slice (when specified/implemented later):** validation must match approved slice scope. Docs-only/templates/checklists: `make format-check`, `make test`, `make docs-check`. Any script/workflow/devcontainer changes: run feasible local checks and report PR CI evidence; do not claim unrun checks. Any tooling/process change affecting on-device validation: include a documented dry-run or real session with evidence when feasible, and state remaining hardware/manual steps explicitly.

## Expected areas/files (future, depending on approved slices)

- `docs/plan/**`, `docs/reports/**`, `docs/README.md` (process/evidence/templates/queue hygiene)
- `.github/workflows/**`, CI helper scripts/config (if CI tiering or parity work approved)
- `.devcontainer/**`, `scripts/**`, `Makefile` targets or docs (if local-env/preflight tooling approved)
- Validation artifacts under `docs/reports/artifacts/**` (if HW evidence workflow formalized)

## Dependencies / prerequisites

- Maintainer agreement on prioritization, acceptable CI/runtime cost, and any process/policy changes (PR flow, required checks, queue semantics).
- Availability of ESP-IDF/QEMU/devcontainer-capable environments and, for HW-focused slices, device access and a reproducible firmware/session setup.
- Coordination with existing DX, QEMU/unblock, and pending hardware-validation items to avoid duplicate/conflicting scope.

## Risks / constraints

- Over-optimizing for CI speed can reduce signal or hide regressions if checks are moved/tiered without care.
- Hardware validation remains constrained by physical access, firmware state, and environment variability; automation can reduce toil but not remove all manual steps.
- Process changes add maintenance burden if not lightweight and aligned with `AGENTS.md`/global gates.
- ESP32 resource constraints mean some bottlenecks (memory/fragmentation/peripheral behavior) are intrinsic and require measurement, not only workflow fixes.

## Open questions (umbrella / first-slice specification)

### Resolved
1. **Priority/scope for v1:** **Resolved (2026-11-20, user).** HW validation lane + process/merge-readiness streamlining.
2. **Single program vs split items:** **Resolved (2026-11-20, user).** Split: umbrella triage remains here; specify/promote a separate first ready slice item.

### Resolved for first slice / deferred for umbrella
3. **First slice boundary & deliverables:** **Resolved (2026-11-20, user).** One combined docs/process ready item (`dx-hw-preflight-and-delivery-evidence-templates`, docs/templates + minimal cross-links only; no scripts/CI/devcontainer/runtime). Core bundle and exact paths/content confirmed: create `docs/reports/templates/hw-validation-preflight.md`, `docs/reports/templates/hw-validation-session.md`, `docs/reports/templates/delivery-evidence-checklist.md`; minimal cross-links in `docs/README.md` and `docs/reports/2026-11-18-pending-hardware-validation-inventory.md`.
4. **Hardware validation definition of “done” / closure model:** **Resolved (2026-11-20, user).** Per-item evidence + explicit human waiver (rationale, approver, context, date); batched sessions may collect evidence but do not replace per-item traceability. Tracking/fields specified in slice.
5. **Ownership/cadence/process rules:** **Resolved (2026-11-20, user, for slice spec).** Recommended practice guidance (not CI-enforced): use preflight+session templates for HW validation passes (batched when pending-HW backlog actionable or before release/milestone closure); complete delivery-evidence checklist for PRs/plan updates claiming merge-ready/done or HW-closure; performer records per-item evidence or explicit waiver. Waiver authority: maintainer/reviewer with rationale recorded.
6. **Success metrics/baseline:** **Resolved (2026-11-20, user, for slice spec).** Adoption/compliance baseline and targets documented (2026-11-18 inventory/report baseline; templates cross-linked; future HW closures use per-item evidence or waiver fields; delivery/finalization uses checklist). Quantitative time/count/age targets deferred to follow-up measurement.
7. **Expected artifacts/areas and constraints:** **Resolved (2026-11-20, user).** Exact create/update paths and no-change areas specified in slice; must remain compatible with `AGENTS.md`, `docs/README.md`, existing HW inventory/reports, and human merge/hardware limits.
8. **Deferred slices (umbrella):** **Resolved as deferred from v1 (2026-11-20, per prioritization).** Integration/QEMU tiering policy, local↔CI parity/devcontainer baseline, and coupled-change guardrails remain out of v1. Promotable later as separate needs-clarification/ready slices with own specs/approvals: integration policy needs suite/flake/latency/signal decisions and CI impact analysis; local↔CI parity needs env/tooling/platform scope decisions; guardrails need concrete checklist/enforcement scope.

### Still open (umbrella)
- **None for current v1 slice.** First slice [`dx-hw-preflight-and-delivery-evidence-templates`](../done/dx-hw-preflight-and-delivery-evidence-templates.md) is **done (delivered, 2026-11-20)** — implemented in PR [#103](https://github.com/gallayl/cos/pull/103) with CI run [44722447791](https://github.com/gallayl/cos/actions/runs/44722447791) green; pending human review/approval/merge. This umbrella remains in `needs-clarification` for future slice triage/specification as needed (deferred: integration/QEMU tiering, local↔CI parity/devcontainer, coupled-change guardrails).

### Previously listed (deferred from v1 unless pulled into slice)
- **Integration policy (former Q3):** deferred from v1; candidate for a later separate slice.
- **Environment baseline/devcontainer canonical path (former Q5):** deferred from v1 (overlaps local↔CI parity); candidate for a later separate slice.

## Progress / evidence

- 2026-11-20: Drafted from recent delivery-bottleneck analysis (git/CI/docs/reports). Item created in `needs-clarification` pending prioritization/specification and explicit approval; no implementation started.
- 2026-11-20 (clarify-feature): Clarification started. Reviewed target + `docs/README.md`, Ready contract, consolidated plan cross-link, related DX/QEMU/HW-validation docs/reports, and ready-item structure. Open questions 1–7 identified; item remained **needs-clarification** (not Ready).
- 2026-11-20 (clarify-feature, round 1, user): decisions recorded — **v1 priority = HW validation + process streamlining**, **shape = split (umbrella triage + separate first ready slice)**, **session goal = drive first chosen slice to Ready now**. Umbrella updated to triage-record framing; Q1/Q2 resolved, remaining open questions narrowed/refined for first-slice specification. Status remains **needs-clarification**.
- 2026-11-20 (clarify-feature, round 2, user): first-slice decisions recorded — **one combined docs/process item** (docs/templates + minimal cross-links only; no scripts/CI/devcontainer/runtime), **Core docs/templates** bundle (HW preflight checklist; batched HW validation session + per-session evidence/log template linked to existing inventory/reports; PR/plan evidence & finalization checklist/template; minimal cross-links), **HW closure = per-item evidence + explicit human waiver** (rationale/approver/context/date; batched sessions do not replace per-item traceability). Created separate slice item [`dx-hw-preflight-and-delivery-evidence-templates`](./dx-hw-preflight-and-delivery-evidence-templates.md) for specification. Status remains **needs-clarification**.
- 2026-11-20 (clarify-feature, round 3, user): confirmed slice ID/title/paths/cross-links, recommended ownership/cadence/waiver guidance, adoption/compliance baseline+targets (quantitative targets deferred), and recommended content scope. Slice spec completed with all Ready-contract fields satisfied and **Open questions: none**; deferred v1 slices recorded with rationale in umbrella.
- 2026-11-20 (clarify-feature, user approval): explicit approval recorded (user, clarify-feature conversation) to mark [`dx-hw-preflight-and-delivery-evidence-templates`](../done/dx-hw-preflight-and-delivery-evidence-templates.md) **ready (approved)** and move it to `docs/plan/ready/` (pre-approved for implementation/PR flow per global gates; merge remains human). Umbrella and slice cross-links updated at that time. No implementation/checks run yet. Umbrella remains **needs-clarification** (triage record for deferred/future slices).
- 2026-11-20 (implementation/delivery): first slice implemented on branch `docs/dx-hw-delivery-evidence-templates` (commit `b852cc4`), PR [#103](https://github.com/gallayl/cos/pull/103) opened/updated and CI run [44722447791](https://github.com/gallayl/cos/actions/runs/44722447791) green (docs-only scope; local `make format-check`, `make test`, `make docs-check` pass). Independent reviewer reported no actionable findings. Item moved to `docs/plan/done/` with evidence; pending human review/approval/merge.