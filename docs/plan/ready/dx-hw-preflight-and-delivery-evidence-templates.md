# DX: HW validation preflight and delivery evidence templates

- **ID:** dx-hw-preflight-and-delivery-evidence-templates
- **Status:** ready (approved)
- **Created:** 2026-11-20
- **Updated:** 2026-11-20
- **Source/links:**
  - Umbrella triage: [`dx-delivery-acceleration-bottlenecks.md`](../needs-clarification/dx-delivery-acceleration-bottlenecks.md) (v1 first slice; split decision)
  - Evidence/baselines: [`docs/reports/2026-11-11-dx1-baseline-measurements.md`](../../reports/2026-11-11-dx1-baseline-measurements.md), [`docs/reports/2026-11-18-pending-hardware-validation-inventory.md`](../../reports/2026-11-18-pending-hardware-validation-inventory.md), [`docs/reports/2026-11-19-hw-validation-only-missing-com3.md`](../../reports/2026-11-19-hw-validation-only-missing-com3.md), [`docs/reports/artifacts/hw-pending-2026-11-18/`](../../reports/artifacts/hw-pending-2026-11-18/)
  - Process/constraints: [`docs/README.md`](../../README.md), root [`AGENTS.md`](../../../AGENTS.md), `.agents/skills/global-gates/SKILL.md`, `.agents/skills/on-device-validation/SKILL.md`
  - Related: [`docs/plan/done/dx1-canonical-baseline-report.md`](../done/dx1-canonical-baseline-report.md)
  - Consolidated tracker: [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md)
- **Approval:** Approved by user in clarify-feature conversation on 2026-11-20 ("Approve and move to ready now"). Pre-approved for implementation/PR flow per global gates (merge remains human).
- **Open questions:** none

## Summary

Create a docs-only, reusable workflow/template set to reduce hardware-validation setup/traceability friction and PR/plan finalization churn. The slice standardizes (1) HW preflight and batched validation session evidence capture with per-item traceability linked to existing inventory/reports, and (2) PR/plan delivery evidence and finalization checklists. Actual device execution, waivers, review/approval, and merge remain human actions.

## Scope

- **In scope (docs/templates + minimal cross-links only):**
  - Create **`docs/reports/templates/hw-validation-preflight.md`**: reusable HW preflight checklist template. Required baseline fields/sections: serial port/baud, flashed firmware identity (commit/build/ELF SHA as applicable), boot profile, storage/SD state, reboot acceptable?, required environment/peripherals/network, tooling prerequisites (including `pyserial` where automation is used), known blockers/affected-build checks, no-`/flash`-writes rule, and quick read-only probe/command-plan references.
  - Create **`docs/reports/templates/hw-validation-session.md`**: batched HW validation session + per-session evidence/log template. Required baseline fields/sections: session metadata, firmware/environment context, command-plan/log/artifact paths, per-item outcome mapping (item/plan ID → checks/commands → evidence paths/logs → outcome `pass`/`fail`/`inconclusive`/`waived` → notes), anomalies/blockers, and waiver fields (rationale, approver, context, date) when outcome is waived.
  - Create **`docs/reports/templates/delivery-evidence-checklist.md`**: PR/plan evidence & finalization checklist/template. Required baseline fields/sections: exact local checks run with outcomes, CI run IDs/URLs, HW/manual validation status per item with evidence path or explicit waiver reference, pending human actions (review/approval/merge/hardware), and plan/queue/cross-link/finalization steps consistent with `docs/README.md` and `AGENTS.md`.
  - Add **minimal cross-links** in `docs/README.md` and `docs/reports/2026-11-18-pending-hardware-validation-inventory.md` pointing to the new templates/workflow (without contradicting existing process/gates).
  - Document **closure policy** in templates: per-item evidence required for HW/manual closure; explicit human waiver path allowed with rationale, approver, and context recorded (and date); batched sessions may collect evidence but do not replace per-item traceability.
- **Implementation form:** markdown templates/checklists and doc cross-link updates only. Minor wording/structure adjustments are allowed during implementation if consistent with this spec, `AGENTS.md`, `docs/README.md`, and recorded decisions.

## Non-goals

- No scripts, CI/workflow changes, devcontainer/tooling changes, firmware/frontend/runtime behavior changes, or dependency upgrades.
- No automated execution of hardware validation and no claiming checks/hardware results beyond cited/recorded evidence.
- No weakening of human merge/approval gates or required automated checks defined by `AGENTS.md`/CI (templates must reflect, not weaken, them).
- No CI enforcement added in this slice.
- Deferred (umbrella, out of v1): integration/QEMU tiering policy, local↔CI parity/devcontainer baseline, coupled-change guardrails.

## Ownership, cadence, and waiver authority

- **Process framing:** recommended practice guidance (docs/templates), not a CI-enforced gate in this slice.
- **Usage/cadence (recommended):** use preflight + session templates for on-device/HW validation passes (batched when pending-HW backlog items are actionable or before release/milestone closure). Complete the delivery-evidence checklist for PRs/plan updates claiming merge-ready/done or HW-closure.
- **Responsibility:** the person performing validation records per-item evidence or an explicit waiver (rationale, approver, context, date) in the relevant session/report/item/PR context using the template fields.
- **Waiver/exception authority:** maintainer/reviewer, with rationale recorded in the relevant item/report/PR.

## Success metrics / baseline

- **Baseline (evidence):** `docs/reports/2026-11-18-pending-hardware-validation-inventory.md` documents pending HW-validation items and prepared session workflow; recent delivery evidence shows ad-hoc follow-up evidence/finalization updates/churn (see umbrella and related reports).
- **Success targets (qualitative/adoption for this docs-only increment):**
  - Templates/checklists exist at the specified paths and are cross-linked from the specified docs.
  - Future HW-closure claims can record per-item evidence or explicit waiver fields using the session template (batched sessions do not replace per-item traceability).
  - Merge-ready/done/finalization updates can be documented using the delivery-evidence checklist with exact checks, CI evidence/links, HW/manual status, and pending human actions.
- **Deferred:** quantitative time/count/age reduction targets (e.g. pending-HW backlog age/count, session setup time, follow-up commit/PR reduction) deferred to follow-up measurement after adoption.

## Acceptance criteria / definition of done

- [x] `docs/reports/templates/hw-validation-preflight.md` created with required baseline preflight fields/sections and closure/context notes consistent with this spec.
- [x] `docs/reports/templates/hw-validation-session.md` created with batched-session metadata, per-item outcome mapping, evidence path fields, and explicit waiver fields (rationale, approver, context, date) supporting per-item traceability.
- [x] `docs/reports/templates/delivery-evidence-checklist.md` created with required local-check/CI/HW-manual-status/pending-human-actions/queue-finalization fields consistent with `docs/README.md` and `AGENTS.md`.
- [x] Minimal cross-links added in `docs/README.md` and `docs/reports/2026-11-18-pending-hardware-validation-inventory.md` to the new templates without contradicting current process/gates.
- [x] Ownership/cadence/waiver guidance and success-metric/baseline statements in this item are reflected in template/doc content as applicable (no open questions).
- [x] `make format-check`, `make test`, and `make docs-check` run and reported with exact pass/fail outcomes for the committed docs-only change (see Progress / evidence).
- [ ] Cross-links updated as applicable (`dx-delivery-acceleration-bottlenecks.md`, consolidated plan if applicable, this item). On delivery, move to `docs/plan/done/` with evidence per `docs/README.md` (merge remains a human action). Pending: PR delivery, required PR CI green per scope, then finalize/move to `docs/plan/done/` with evidence.

## Validation plan

- **Automated (required for committed docs-only scope):** from repo root run and report exact outcomes: `make format-check`, `make test`, `make docs-check`.
- **Manual review (required):** verify templates are internally consistent, reference correct existing paths/process rules (`AGENTS.md`, `docs/README.md`, HW inventory/reports), include required fields/closure policy, and do not claim/require unapproved automation or weaken gates.
- **On-device/hardware validation:** not required for this docs/templates slice. Templates must explicitly record remaining hardware/manual steps and evidence/waiver handling when later used.
- **Dry-run:** optional if feasible — apply templates to structure one real or retrospective example without claiming new device evidence; report if performed.

## Expected areas/files

- **Create:**
  - `docs/reports/templates/hw-validation-preflight.md`
  - `docs/reports/templates/hw-validation-session.md`
  - `docs/reports/templates/delivery-evidence-checklist.md`
- **Update (minimal cross-links/state):**
  - `docs/README.md`
  - `docs/reports/2026-11-18-pending-hardware-validation-inventory.md`
  - `docs/plan/needs-clarification/dx-delivery-acceleration-bottlenecks.md` (umbrella cross-link/status updates, if applicable)
  - `docs/plan/ready/dx-hw-preflight-and-delivery-evidence-templates.md` (this item)
  - `docs/plan/consolidated-improvement-plan.md` only if adding/updating a tracker cross-link is deemed applicable
- **Must not change:** CI workflows, scripts, devcontainer, firmware/frontend/runtime code, or other ready-queue items.

## Dependencies / prerequisites

- Existing process sources remain authoritative: `AGENTS.md`, `docs/README.md`, `.agents/skills/global-gates/SKILL.md`, `.agents/skills/on-device-validation/SKILL.md`, and existing HW inventory/reports.
- Human availability for any later hardware validation sessions that use the templates.
- `make format-check`, `make test`, and `make docs-check` runnable in the implementation environment (report blockers per `AGENTS.md` if not).

## Risks / constraints

- **Template drift/maintenance:** templates can become stale if process/CI/gates change; mitigate with links to authoritative docs and lightweight scope.
- **Over-prescription:** rigid checklists may add overhead; keep focused on observed pain (preflight, per-item traceability, delivery/finalization evidence).
- **False closure risk:** waiver path must require explicit rationale, approver, context, and date to avoid silent skipping of required evidence.
- **Scope creep:** resist adding scripts/automation/CI/policy changes in this docs-only slice unless re-scoped and re-approved.
- **No hardware implication:** this slice does not create or validate device evidence; hardware/manual validation remains a human-gated activity when required.

## Progress / evidence

- 2026-11-20: Created from umbrella `dx-delivery-acceleration-bottlenecks.md` after user decisions (v1 = HW validation + process streamlining; split; drive first slice to Ready; combined docs-only core templates; per-item evidence + explicit human waiver). Draft spec added; open questions recorded.
- 2026-11-20 (clarify-feature, round 3, user): confirmed ID/title/paths/cross-links, recommended ownership/cadence/waiver guidance, adoption/compliance baseline+targets (quantitative targets deferred), and recommended content scope. Spec completed with all Ready-contract fields satisfied and **Open questions: none**.
- 2026-11-20 (clarify-feature, user approval): explicit approval recorded (user, clarify-feature conversation) to mark ready and move to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates (merge remains human). No implementation/checks run yet.
- 2026-11-20 (implementation): implemented docs-only scope on branch `docs/dx-hw-delivery-evidence-templates` from healthy base `main@5c86e2b` (CI run 44721409172 success). Created `docs/reports/templates/hw-validation-preflight.md`, `docs/reports/templates/hw-validation-session.md`, and `docs/reports/templates/delivery-evidence-checklist.md`; added minimal cross-links in `docs/README.md` and `docs/reports/2026-11-18-pending-hardware-validation-inventory.md`. Consolidated plan/umbrella cross-links reviewed (no additional updates deemed necessary at this step). Manual template review: required fields/closure policy/per-item evidence+waiver guidance present; references authoritative docs (`AGENTS.md`, `docs/README.md`, HW inventory/reports, skills); no scripts/CI/devcontainer/runtime changes. Optional dry-run against a real/retrospective example: **not performed** (no new device evidence claimed). Local checks (repo root, executed): `make format-check` — **pass** (exit 0); `make test` — **pass** (exit 0; CTest 31/31 passed, Python `scripts` unittest 3/3 OK); `make docs-check` — **pass** (exit 0). PR delivery/CI and final move to `docs/plan/done/` pending.
