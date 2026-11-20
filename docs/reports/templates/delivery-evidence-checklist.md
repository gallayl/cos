# PR/plan delivery evidence & finalization checklist (template)

Reusable, docs-only checklist for documenting merge-ready/done claims, PR evidence, and plan/queue finalization. Copy into a PR description section, plan item progress note, or delivery report as applicable. Record only checks/evidence actually executed or cited; do **not** weaken `AGENTS.md`/`docs/README.md` gates.

- **Related:** [`hw-validation-preflight.md`](./hw-validation-preflight.md), [`hw-validation-session.md`](./hw-validation-session.md), [`../../README.md`](../../README.md), root [`AGENTS.md`](../../../AGENTS.md), [`.agents/skills/global-gates/SKILL.md`](../../../.agents/skills/global-gates/SKILL.md), [`.agents/skills/validate-change/SKILL.md`](../../../.agents/skills/validate-change/SKILL.md)
- **Process framing:** recommended practice guidance (docs/templates), **not** a CI-enforced gate in this slice. Merge, review/approval, and required hardware/manual validation remain human actions unless explicitly waived with rationale recorded.

## Usage (recommended)

- **When to complete:** for PRs/plan updates claiming merge-ready, implementation delivered, done/finalization, or HW/manual closure.
- **Who records:** the person delivering/updating the work records exact local checks, CI evidence, HW/manual status, and pending human actions in the PR/item/docs context.
- **Evidence rule:** never claim a check passed unless it was executed. Distinguish locally executed checks from CI-covered checks and from hardware/manual validation.

## 1) Scope and links

| Field | Value |
|---|---|
| PR / branch | `<#number URL or branch>` |
| Base branch / commit | `<base and/or merge base>` |
| Head commit(s) | `<SHA(s)>` |
| Related plan/idea/history/report docs | `<links/paths>` |
| Scope summary | `<what is included>` |
| Non-goals / exclusions | `<what is not included>` |

## 2) Exact local checks run (executed evidence)

List only checks actually run locally in this environment/scope. Include command, result, and brief notes/fixes. If a required check was not run, list it under blockers/remediation instead of as passed.

| Command | Result (pass/fail) | Notes (scope, fixes, reruns) |
|---|---|---|
| `<e.g. make format-check>` | `<pass/fail>` | `<details>` |
| `<e.g. make test>` | `<pass/fail>` | `<details>` |
| `<e.g. make docs-check>` | `<pass/fail>` | `<details>` |
| `<frontend/yarn or other scope checks if run>` | `<pass/fail/n-a>` | `<cwd, node/yarn env, notes>` |

**Environment/tooling blockers (if any):** `<missing ESP-IDF/QEMU/Node/Yarn/gh/etc., what to install/configure, env vars/paths, exact rerun command>`

## 3) CI evidence

| Workflow/check context | Run ID / URL | Status/conclusion | Notes |
|---|---|---|---|
| `<CI / required checks>` | `<run URL or ID>` | `<success/failure/pending/skipped-n/a>` | `<head SHA, relevant jobs, path-filter rationale if skipped>` |

Do not list CI-covered checks as locally executed unless you actually ran them and reporting them is necessary.

## 4) Hardware/manual validation status (per item)

Required for HW/manual closure claims: provide per-item evidence path or explicit waiver reference. Batched sessions do not replace per-item traceability.

| Item / plan ID | Status | Evidence path/log/report | Waiver reference (if waived) | Notes / remaining manual steps |
|---|---|---|---|---|
| `<item>` | `<pass/fail/inconclusive/waived/not-run/n-a>` | `<path/link>` | `<rationale, approver, context, date - see session template>` | `<what remains>` |

If hardware/manual validation is required and unresolved, add/surface a clear blocking PR comment or review thread describing what must be validated until resolved with evidence (or explicitly waived by a human with rationale).

## 5) Pending human actions

- [ ] Review/approval: `<pending/done/n-a>`
- [ ] Merge (human/maintainer action): `<pending/n-a>`
- [ ] Hardware/manual validation: `<pending with evidence/waiver refs or n-a>`
- [ ] Other decisions/access/environment actions: `<details or none>`

## 6) Plan/queue/cross-link/finalization steps

Use as applicable and keep consistent with [`docs/README.md`](../../README.md) and root [`AGENTS.md`](../../../AGENTS.md):

- [ ] Relevant plan/idea/history/report docs updated with scope, validation/evidence, and follow-ups.
- [ ] Cross-links updated (item ↔ reports/artifacts ↔ consolidated plan/umbrella as applicable).
- [ ] Queue/state transitions reflect reality: `ready` only while implementable now; blocked/incomplete items moved/kept in `needs-clarification`; move to `done` only when scoped implementation is complete and delivered per `docs/README.md` with evidence recorded.
- [ ] `done` move (if applicable) does **not** imply merge; pending human review/approval, merge, and required hardware/manual validation are recorded in item/docs/PR.
- [ ] No historical content removed; no unrun checks or hardware validation claimed.

## 7) Final delivery summary

- **Delivered in this PR/change:** `<summary>`
- **Not delivered / out of scope:** `<summary>`
- **Evidence highlights:** `<key logs/reports/run URLs>`
- **Ready for merge from implementation/CI/docs perspective?** `<yes/no/blocked>` (human review/approval/merge still required)

## Baseline/adoption note

This checklist targets observed delivery/finalization and evidence-traceability friction (see `docs/plan/needs-clarification/dx-delivery-acceleration-bottlenecks.md` and related reports). Success for this docs-only slice is adoption/compliance-oriented (templates/cross-links in place; future HW closures and merge-ready/finalization claims can use per-item evidence/waiver and explicit check/CI/HW/pending-action fields). Quantitative improvement targets are deferred to follow-up measurement after adoption.
