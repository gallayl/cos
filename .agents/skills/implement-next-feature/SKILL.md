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
   - **Stop and wait for explicit user approval ("go")** before implementation or Git write operations. If baseline was fixed, resume from a healthy base.
3. **Prepare branch** — compose `prepare-working-branch` (from agreed healthy base; merge base to sync; no destructive rewrites).
4. **Track progress** — compose `track-doc-progress` throughout (source plan/idea/history/report, status/evidence/follow-ups, keep consistent with PR).
5. **Implement, test, and validate** — follow `implementation-workflow` for the approved scope: focused changes, add/update automated tests where possible, handle script-facing API/example sync via `add-device-script-example` when applicable, and compose `testing`/`validate-change` (per root `AGENTS.md`) until required checks pass or blockers are documented with actionable remediation. Provide on-device guide/scripts if hardware validation is required and state what remains.
6. **Deliver PR** — compose `deliver-pr` (Conventional Commits, push, open/update PR—prefer draft while iterating—with minimal description linking docs). **Never merge the PR.**
7. **Iterate to green** — compose `babysit-pr` to keep branch up to date (merge base), drive all PR checks green, and resolve/address review feedback until merge-ready or clearly blocked.
8. **Report final state** — actions taken, commits/PR links, executed checks and outcomes, remaining blockers/human actions (including required review/approval and any hardware validation). Merge (squash expected) is a human/maintainer action.

## Gates (must preserve)

- **Approval:** items in `docs/plan/ready/` that satisfy the Ready contract (explicit approval recorded after specification, `Open questions: none`, implementable now) are **pre-approved** for implementation/PR/babysit-to-green flow (merge remains human). For non-ready/questioned/incomplete items, **stop and wait for explicit user approval ("go")** after proposal before implementation/Git writes; route questioned items to `docs/plan/needs-clarification/` and `clarify-feature` instead of implementing.
- **Green baseline required;** baseline fixes on a separate branch/PR and merged before unrelated feature work.
- **Never auto-upgrade dependencies** unless explicitly in-scope/requested.
- **No force-push/history rewrites, no destructive commands, no merging PRs,** no discarding unrelated user changes.
- Evidence-based validation/reporting per root `AGENTS.md` (never claim unrun checks).
