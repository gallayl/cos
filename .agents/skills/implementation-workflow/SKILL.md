---
name: implementation-workflow
description: Primary entry procedure for implementing a requested change end-to-end (generic). Use for feature work, fixes, and refactors: understand the task, plan, implement incrementally, validate with evidence, request/reconcile review, and verify completion by composing granular skills as needed.
---

# Implementation workflow (primary, generic)

Primary, portable procedure for delivering a requested implementation. This workflow is **generic** (not specific to selecting the next item from `docs/`).

## Workflow

1. **Understand the task and codebase.** Clarify requirements and acceptance criteria; inspect relevant code, docs, and conventions (`AGENTS.md`, nested `AGENTS.md`). Identify affected areas, dependencies, and risks.
2. **Establish baseline and plan.** Ensure a safe starting point (`ensure-green-baseline` when branch/PR delivery or baseline health matters). Define a concrete plan, scope/non-goals, and validation strategy.
3. **Prepare workspace when delivering changes.** If creating commits/PRs, compose `prepare-working-branch` from an agreed healthy base (respect approval/safety gates required by the task).
4. **Implement incrementally.** Make minimal, focused changes consistent with surrounding patterns. Add/update tests and documentation as appropriate. For script-facing SDK/Berry API changes, follow `add-device-script-example`.
5. **Validate with evidence.** Compose `testing` / `validate-change` for scope-appropriate checks; fix failures and re-run. **Never claim a check passed unless executed**; report blockers and remaining hardware/manual steps per root `AGENTS.md`. A background `test-runner` subagent is optional (CI runs tests), but does not replace required local evidence when checks are run.
6. **Track and package work as required.** When the task involves planned/docs-tracked work or PR delivery, compose `track-doc-progress` and `deliver-pr` (and follow repo gates: explicit approval before Git writes when required, no auto-merge, no force-push). For CI/review follow-through, compose `babysit-pr`.
7. **Request/perform independent review.** For substantive changes, run an independent read-only `reviewer` subagent using `code-review`; resolve actionable findings and re-verify. For trivial changes, a direct review may suffice.
8. **Confirm completion.** Verify acceptance criteria against executed results and working tree/diff; report outcomes, evidence, and any remaining risks or manual validation.

## Layering and gates

- **Primary entry procedure:** composes granular skills; do not duplicate their procedures.
- Respect root `AGENTS.md` and composed-skill gates: green baseline (separate baseline-fix branch/PR merged before unrelated feature work when applicable), explicit user approval before implementation/Git writes when required, safe Git (merge base to sync, no history rewrites), dependency discipline, and evidence-based reporting.
- For the repo-specific "implement next feature from docs" flow, use `pick-next-work-item` and `implement-next-feature` (composed workflow) as the explicit entry point.
