---
name: pick-next-work-item
description: Granular procedure to select and propose the next sensible, scoped work item from docs/ with a validation and Git/PR plan. Prioritizes the ready queue; treats fully specified, approved ready items as pre-approved and stops for explicit approval ("go") for non-ready or questioned items.
---

# Pick next work item (granular)

Select one complete, well-scoped item from repository documentation and propose it clearly. **Granular skill** composed by repo-specific workflows/primary procedures.

## State-based selection

- **Ready queue (highest priority):** `docs/plan/ready/*.md` — items that meet the Ready contract (explicit approval recorded after specification, `Open questions: none`, scope/non-goals, acceptance criteria/DoD, validation plan, dependencies/risks) and are **implementable now** (no blockers). `plan/ready/` must contain only ready-to-implement items.
- **Needs clarification:** `docs/plan/needs-clarification/*.md` — items with open questions/blockers or incomplete spec; **not** pickable for implementation until clarified and moved to `ready/` (use `clarify-feature` / `/clarify-feature`).
- **Done:** `docs/plan/done/*.md` — completed plan items with recorded evidence; not pickable for implementation.
- **Canonical tracker:** `docs/plan/consolidated-improvement-plan.md` remains the source of truth for planned/current status and evidence (link items both ways).

## Workflow

1. **Read doc structure and status rules:** `docs/README.md` and the Ready/needs-clarification contract.
2. **Check baseline/context as available:** repo state, default branch, and CI/remote health if determinable (prefer `gh` when available). If baseline is red, prefer proposing baseline restoration as the priority item (separate baseline-fix branch/PR) per `ensure-green-baseline`.
3. **Review candidates (in order):**
   - `docs/plan/ready/*.md` (pre-approved, implementable now)
   - `docs/plan/consolidated-improvement-plan.md` and other active `docs/plan/*.md` for tracked work that may need promotion/clarification
   - `docs/ideas/*.md` (drafts; pick only if self-contained/sensible now and clarified/approved as needed)
   - relevant `docs/history/*.md` and `docs/reports/*.md` for context/evidence
   - **Process all relevant doc markdown files** when assessing pending work; do not assume the consolidated plan lists every item.
4. **Pick one complete item** that is clearly scoped (acceptance criteria/DoD), not blocked by unavailable prerequisites (unless deliverable is prep/guide/script), high value, and compatible with a healthy baseline. Prefer items in `ready/`.
5. **Report the proposal:** source doc(s) and item id/heading; scope and non-goals; expected files/areas; validation plan (automated checks and any required on-device validation); Git/PR plan (base branch, proposed branch/intent) and any necessary dependency/tooling considerations; assumptions, risks, blockers, and baseline/CI status.
6. **Gate (approval):**
   - **Ready & compliant:** if the selected item is in `docs/plan/ready/` and satisfies the Ready contract (explicit approval, no open questions, implementable now), treat it as **pre-approved** for implementation/PR flow (merge remains human) and proceed without a per-item "go".
   - **Otherwise:** **stop and wait for explicit user approval ("go")** before implementation or Git write operations. If the item has open questions/blockers or incomplete spec, route to `docs/plan/needs-clarification/` and use `clarify-feature` / `/clarify-feature` instead of implementing.

## Constraints

- Keep proposals factual and aligned with canonical plan/status; do not remove historical content.
- Do not invent approvals or mark questioned/blocked items ready.
- Follow root `AGENTS.md` for dependency/update discipline and validation/reporting expectations.
