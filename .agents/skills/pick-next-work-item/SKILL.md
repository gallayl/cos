---
name: pick-next-work-item
description: Granular procedure to select and propose the next sensible, scoped work item from docs/ with a validation and Git/PR plan. Use when composing docs-driven planning flows; stops for explicit approval ("go") before implementation/Git writes where required.
---

# Pick next work item (granular)

Select one complete, well-scoped item from repository documentation and propose it clearly. **Granular skill** composed by repo-specific workflows/primary procedures.

## Workflow

1. **Read doc structure and status rules:** `docs/README.md`.
2. **Review candidates (in order):**
   - `docs/plan/consolidated-improvement-plan.md` (canonical tracker)
   - other active `docs/plan/*.md`
   - `docs/ideas/*.md` (only if self-contained and sensible now)
   - relevant `docs/history/*.md` and `docs/reports/*.md` for context/evidence
3. **Check baseline/context as available:** repo state, default branch, and CI/remote health if determinable (prefer `gh` when available). If baseline is red, prefer proposing baseline restoration as the priority item (separate baseline-fix branch/PR) per `ensure-green-baseline`.
4. **Pick one complete item** that is clearly scoped (acceptance criteria/DoD), not blocked by unavailable prerequisites (unless deliverable is prep/guide/script), high value, and compatible with a healthy baseline.
5. **Report the proposal:** source doc(s) and item id/heading; scope and non-goals; expected files/areas; validation plan (automated checks and any required on-device validation); Git/PR plan (base branch, proposed branch/intent) and any necessary dependency/tooling considerations; assumptions, risks, blockers, and baseline/CI status.
6. **Gate:** **Stop and wait for explicit user approval ("go") before implementation or Git write operations** when the workflow requires it. Ask for clarification if needed to choose/scope the right item.

## Constraints

- Keep proposals factual and aligned with canonical plan/status; do not remove historical content.
- Follow root `AGENTS.md` for dependency/update discipline and validation/reporting expectations.
