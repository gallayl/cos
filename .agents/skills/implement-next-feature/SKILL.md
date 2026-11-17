---
name: implement-next-feature
description: Repo-specific composed workflow to pick and deliver the next sensible feature/task from docs/ through a safe baseline/branch/validation/PR/CI flow. Use when asked to continue planned work or implement the next documented item (composes primary and granular skills; not the generic /implement command).
---

# Implement next feature from docs (composed)

Repo-specific workflow that composes the **generic** `implementation-workflow` with granular skills to select and deliver one documented work item. Use this as an explicit entry point for "next planned/docs item" requests; for arbitrary implementation requests prefer the generic `implementation-workflow` (and `/implement` where available).

## Composed flow

1. **Preflight and green baseline** — compose `ensure-green-baseline`.
   - Establish repo state/base branch/CI health with evidence. If baseline is red, restore via a **separate baseline-fix branch/PR** and **pause for merge** before unrelated feature work.
2. **Pick and propose** — compose `pick-next-work-item`.
   - Select one complete, sensible item from `docs/` (see `docs/README.md`, canonical plan first) with scope/non-goals, expected areas, validation plan, and Git/PR plan.
   - **Approval gate:** if the selected item is in `docs/plan/ready/` and satisfies the Ready contract per `pick-next-work-item`/Gates (pre-approved for implementation/PR/babysit-to-green; merge remains human), proceed without a per-item `"go"`. Otherwise (non-ready, questioned, incomplete, or not Ready-compliant), **stop and wait for explicit user approval ("go")** before implementation or Git write operations, or route to `docs/plan/needs-clarification/` + `clarify-feature` as appropriate. If baseline was fixed, resume from a healthy base (ready-preapproved items still do not require a new `"go"`).
3. **Prepare branch** — compose `prepare-working-branch` (from agreed healthy base; merge base to sync; no destructive rewrites).
4. **Track progress** — compose `track-doc-progress` throughout (source plan/idea/history/report, status/evidence/follow-ups, keep consistent with PR). Keep the item in `docs/plan/ready/` only while it remains ready-to-implement.
5. **Implement, test, and validate** — follow `implementation-workflow` for the approved scope: focused changes, add/update automated tests where possible, handle script-facing API/example sync via `add-device-script-example` when applicable, and compose `testing`/`validate-change` (per root `AGENTS.md`) until required checks pass or blockers are documented with actionable remediation. Run validation directly as needed; do not require a background `test-runner` subagent (CI runs the tests), but still report executed local evidence per root `AGENTS.md`. Provide on-device guide/scripts if hardware validation is required and state what remains.
6. **Independent review loop** — for substantive changes, run an independent review via the read-only `reviewer` subagent (`code-review` skill). Resolve actionable findings, re-validate as needed, record outcomes/evidence, then **re-run review**. Repeat until no valid/actionable findings remain or a hard blocker/decision is required (documented). For trivial changes, a direct review may suffice.
7. **Deliver PR** — compose `deliver-pr` (Conventional Commits, push, open/update PR—prefer draft while iterating—with minimal description linking docs). **Never merge the PR.**
8. **Iterate to green** — **immediately after step 7 in the same run** (do not stop at PR creation), compose `babysit-pr` to keep branch up to date (merge base), drive all PR checks green, and resolve/address review feedback until merge-ready or clearly blocked (or a hard blocker is documented).
9. **Finalize doc state** — when the scoped implementation is complete and delivered (PR opened/updated, required automated checks/PR CI green per scope, and validation/evidence recorded in docs), move the item from `docs/plan/ready/` to `docs/plan/done/` (update status/links per `track-doc-progress`). Record remaining human actions (review/approval, merge) and any required hardware/manual validation in the item/docs and PR; moving to `done/` does not imply merge. If it becomes blocked/not ready-to-implement before completion, move it to `docs/plan/needs-clarification/` instead.
10. **Report final state** — only after step 8 reaches `babysit-pr`'s definition of done or a hard blocker is documented, report actions taken, commits/PR links, executed checks and outcomes, reviewer findings/resolution, and remaining blockers/human actions (including required review/approval, merge, and any hardware/manual validation). If hardware/manual validation is required, ensure it is called out as a blocking PR comment/thread until resolved with evidence (or explicitly waived by a human). Merge (squash expected) is a human/maintainer action.

## Gates (must preserve)

- Follow `global-gates` and root `AGENTS.md` (approval/ready, green baseline, safe Git, dependency discipline, evidence-based validation/reporting, human merge/hardware limits).
- **Ready-specific approval:** items in `docs/plan/ready/` that satisfy the Ready contract (explicit approval recorded after specification, `Open questions: none`, implementable now) are **pre-approved** for implementation/PR/babysit-to-green flow (merge remains human). For non-ready/questioned/incomplete items, **stop and wait for explicit user approval ("go")** after proposal before implementation/Git writes; route questioned items to `docs/plan/needs-clarification/` and `clarify-feature` instead of implementing. If baseline was fixed, resume from a healthy base (ready-preapproved items still do not require a new `"go"`).
