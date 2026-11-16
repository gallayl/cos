# COS Documentation

This directory is organized so you can quickly see what is **done/historical**, what is **planned/structured**, what is **ready to implement**, and what still needs **clarification**.

## Structure

| Area | Path | Purpose | Status meaning |
|---|---|---|---|
| **Plan (living)** | [`plan/consolidated-improvement-plan.md`](./plan/consolidated-improvement-plan.md) | Cross-cutting tracker for priorities, ordering, acceptance criteria, progress, and follow-ups. | **Source of truth for current status** of planned/remaining work and evidence. Links to queue items when applicable. |
| **Ready queue** | [`plan/ready/`](./plan/ready/) | Items that are fully specified, **approved (after specification), have no open questions**, and are **ready to implement now** (not blocked). | **Only ready-to-implement items live here.** Pre-approved for implementation/PR flow when they satisfy the Ready contract (merge remains human). Picked first by docs-driven workflows. |
| **Needs clarification** | [`plan/needs-clarification/`](./plan/needs-clarification/) | Items with open questions, missing spec/approval, or blockers that prevent immediate implementation. | **Not ready.** Must be clarified (e.g. `clarify-feature` / `/clarify-feature`) and meet the Ready contract before moving to `plan/ready/`. |
| **Done queue** | [`plan/done/`](./plan/done/) | Plan items whose implementation is complete and delivered with recorded validation/evidence (or explicitly closed with documented outcome). | **No longer ready-to-implement work items.** Move here when implementation is ready for merge (PR opened/updated, required automated checks/PR CI green per scope, and docs/evidence updated). Pending human actions (review/approval, merge, or required hardware/manual validation) must be recorded in the item and surfaced on the PR; merge remains a human action. Keep status/evidence/follow-up links; do not remove history. |
| **Plan details** | [`plan/`](./plan/) | Detailed/active plan documents and trackers. | Planning/spec context; actionable items should be represented in `ready/`, `needs-clarification/`, or `done/` as appropriate. |
| **History / specs** | [`history/`](./history/) | Dated improvement plans, design specs, and conformance documents. | **Historical design/spec records.** Many delivered; some have follow-ups tracked in plan queues. Treat as background/evidence, not the live queue. |
| **Ideas / drafts** | [`ideas/`](./ideas/) | New proposals not yet fully scheduled or accepted. | **Draft/idea state.** Clarify and promote deliberately. |
| **Reports / evidence** | [`reports/`](./reports/) | Hardware validation guides/results, profiling checklists, audits, focused reports. | **Validation evidence.** May imply open follow-ups (track in needs-clarification/plan as needed). |
| **Progress notes** | [`progress/`](./progress/) | Dated progress tracking for specific workstreams. | Supporting progress evidence; actionable follow-ups should be reflected in plan queues/tracker. |
| **Review findings** | [`review-findings/`](./review-findings/) | Audits/reviews and findings. | Evidence/findings; convert actionable work into clarified items when appropriate. |

## Ready contract (state transition)

An item in `plan/ready/` must document: **ID/title**, **status `ready` (approved)**, **source/links**, **approval** (who/when/context, after specification), **open questions: none**, **scope/non-goals**, **acceptance criteria/DoD**, **validation plan** (automated per `AGENTS.md`; hardware/manual only when the deliverable is the guide/script or explicitly required and feasible), **expected areas/files**, **dependencies/prerequisites**, and **risks/constraints**. `plan/ready/` contains only items that satisfy this contract and are **implementable now**. Items missing any of these, or blocked by unavailable decisions/hardware/external dependencies, belong in `plan/needs-clarification/`.

## Quick guide

- **Want the next implementable, pre-approved work?** Check [`plan/ready/`](./plan/ready/) first, then see [`plan/consolidated-improvement-plan.md`](./plan/consolidated-improvement-plan.md).
- **Want to clarify/open questions or create a spec?** Use [`plan/needs-clarification/`](./plan/needs-clarification/) with the `clarify-feature` skill / `/clarify-feature` (and `/new-feature` to create a standardized item).
- **Want completed plan items and their evidence?** See [`plan/done/`](./plan/done/) and [`plan/consolidated-improvement-plan.md`](./plan/consolidated-improvement-plan.md).
- **Want the current overall picture?** Start with [`plan/consolidated-improvement-plan.md`](./plan/consolidated-improvement-plan.md).
- **Want detailed/historical context or evidence?** See [`plan/`](./plan/), [`history/`](./history/), [`ideas/`](./ideas/), [`reports/`](./reports/), [`progress/`](./progress/), and [`review-findings/`](./review-findings/).

## Maintenance rules

- **No data loss:** keep historical docs and reports; do not delete evidence.
- **Process all doc markdown files** when assessing pending/actionable work (not only the consolidated plan). Keep the consolidated plan linked/consistent as the canonical status tracker.
- **State transitions:** clarify and record approval after specification before moving an item to `plan/ready/`; keep only ready-to-implement items in `plan/ready/` (move or keep items with open questions/blockers in `plan/needs-clarification/`; move items whose scoped implementation is complete and delivered with recorded evidence to `plan/done/` once required automated checks/PR CI are green per scope and the PR is up to date).
- **Done items and remaining gates:** moving to `plan/done/` does not imply merge. Record pending human review/approval, merge, and any required hardware/manual validation in the item. If hardware/manual validation is required, add a clear PR comment/review thread describing what must be validated and treat it as a merge-blocking item until resolved with evidence (or explicitly waived by a human with rationale).
- **Update status/evidence** when work lands; link validation evidence and cross-link queue items to source docs and the consolidated plan. When moving an item between queues, update its status and cross-links in the same change.
- **Prefer stable links:** when adding new docs, place them in the appropriate area and cross-link as needed.
