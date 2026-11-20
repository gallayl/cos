# DX: Delivery acceleration from recent CI/validation bottlenecks

- **ID:** dx-delivery-acceleration-bottlenecks
- **Status:** needs-clarification
- **Created:** 2026-11-20
- **Source/links:**
  - Evidence (this draft is derived from): `git log --since="5 days ago"` / PRs `fb458ad`…`5ec95d3` (2026-11-17→2026-11-20), `.github/workflows/ci.yaml` and reusable workflows, `docs/plan/consolidated-improvement-plan.md`, `docs/plan/ready/` (5 items), `docs/plan/needs-clarification/` (17 items at time of draft)
  - Reports: [`docs/reports/2026-11-11-dx1-baseline-measurements.md`](../../reports/2026-11-11-dx1-baseline-measurements.md), [`docs/reports/2026-11-18-pending-hardware-validation-inventory.md`](../../reports/2026-11-18-pending-hardware-validation-inventory.md), [`docs/reports/2026-11-19-hw-validation-only-missing-com3.md`](../../reports/2026-11-19-hw-validation-only-missing-com3.md)
  - Related: [`docs/plan/needs-clarification/developer-tooling-dx-improvement-plan.md`](./developer-tooling-dx-improvement-plan.md), [`docs/plan/done/dx1-canonical-baseline-report.md`](../done/dx1-canonical-baseline-report.md), [`docs/plan/needs-clarification/unblock-shell-fs-pipeline-and-qemu-hangs.md`](./unblock-shell-fs-pipeline-and-qemu-hangs.md), pending hardware-validation items listed in the 2026-11-18 inventory
- **Approval:** pending (explicit approval required after specification; not Ready/pre-approved)

## Summary

Draft a prioritized, evidence-backed improvement program to reduce cycle time from implementation to merge-ready/delivered features. Focus areas observed in recent work: hardware/manual validation tail and setup fragility, local↔CI parity and expensive checks, integration/QEMU cost and flakiness, and plan/PR/finalization overhead. This item captures candidate slices and open questions; it does **not** implement tooling/CI/runtime changes.

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

## Non-goals (for this draft)

- Implementing CI workflow changes, scripts, devcontainer/tooling, firmware/frontend/runtime behavior, or dependency upgrades in this item.
- Declaring specific improvements approved, scheduled, or complete.
- Claiming checks or hardware validation were executed beyond evidence cited from existing reports/history.
- Marking this item Ready without satisfying the Ready contract and explicit approval after specification.

## Acceptance criteria (for clarifying/moving this item toward Ready)

- [ ] Prioritized scope selected (which slices/increments are in v1, and which are deferred/split into separate items).
- [ ] For each in-scope slice: concrete deliverables, owner/process or automation target, success metrics/baseline, and validation/rollout plan defined.
- [ ] Dependencies/prerequisites and risks documented (CI cost, maintainer burden, hardware availability, policy/process changes).
- [ ] Open questions resolved and **Open questions: none** recorded; explicit approval (who/when/context) captured after specification.
- [ ] Item placed in correct queue/state with consolidated-plan cross-links updated consistently.

## Validation plan (when implementation slices are later approved)

- Docs-only/planning changes: `make format-check`, `make test`, `make docs-check` (and report exact outcomes).
- CI/workflow or script changes (if later in scope): run relevant local checks feasible in environment, validate workflow behavior where possible, and rely on/report PR CI evidence; do not claim unrun checks.
- Tooling/process changes affecting on-device validation: update inventory/plans/templates and record at least one executed dry-run or real session with evidence when feasible.
- Hardware/manual validation: only where a deliverable requires it; state remaining steps explicitly.

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

## Open questions

1. **Priority/scope for v1:** which 1–2 slices should be tackled first (HW validation batching, local/CI parity, integration tiering, or process streamlining)?
2. **Single program vs split items:** keep as one umbrella needs-clarification item, or split into separate promotable items per slice once prioritized?
3. **Integration policy:** which QEMU/integration suites (if any) are candidates for required-smoke vs extended/nightly/quarantine, and what is the acceptable flake/latency budget?
4. **Hardware validation definition of “done”:** for items requiring HW/manual evidence, should closure require a scheduled batched pass, per-item evidence, or explicit human waiver workflow (and how tracked)?
5. **Environment baseline:** should devcontainer be the canonical local path for build/lint/integration, with native Windows documented as best-effort?
6. **Ownership/cadence:** who runs/triages HW validation batches and CI-flake follow-ups, and at what cadence/trigger?
7. **Success metrics:** which DX1/CI/hardware metrics should be treated as baseline targets (e.g. lint/build/integration times, flake rate, time-to-merge-ready, pending-HW backlog age)?

## Progress / evidence

- 2026-11-20: Drafted from recent delivery-bottleneck analysis (git/CI/docs/reports). Item created in `needs-clarification` pending prioritization/specification and explicit approval; no implementation started.
