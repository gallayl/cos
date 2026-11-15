# COS Documentation

This directory is organized so you can quickly see what is **done/historical**, what is **planned/structured**, what is **ready to implement**, and what still needs **clarification**.

## Structure

| Area | Path | Purpose | Status meaning |
|---|---|---|---|
| **Plan (living)** | [`plan/consolidated-improvement-plan.md`](./plan/consolidated-improvement-plan.md) | Cross-cutting tracker for priorities, ordering, acceptance criteria, progress, and follow-ups. | **Source of truth for current status** of planned/remaining work and evidence. Links to ready/needs-clarification items when applicable. |
| **Ready queue** | [`plan/ready/`](./plan/ready/) | Items that are fully specified, **approved (after specification), have no open questions**, and are implementable now. | **Pre-approved for implementation/PR flow** when they satisfy the Ready contract (merge remains human). Picked first by docs-driven workflows. |
| **Needs clarification** | [`plan/needs-clarification/`](./plan/needs-clarification/) | Items with open questions, missing spec/approval, or blockers that prevent immediate implementation. | **Not ready.** Must be clarified (e.g. `clarify-feature` / `/clarify-feature`) and meet the Ready contract before moving to `plan/ready/`. |
| **Plan details** | [`plan/`](./plan/) | Detailed/active plan documents and trackers. | Planning/spec context; actionable items should be represented in `ready/` or `needs-clarification/` as appropriate. |
| **History / specs** | [`history/`](./history/) | Dated improvement plans, design specs, and conformance documents. | **Historical design/spec records.** Many delivered; some have follow-ups tracked in plan/ready/needs-clarification. Treat as background/evidence, not the live queue. |
| **Ideas / drafts** | [`ideas/`](./ideas/) | New proposals not yet fully scheduled or accepted. | **Draft/idea state.** Clarify and promote deliberately. |
| **Reports / evidence** | [`reports/`](./reports/) | Hardware validation guides/results, profiling checklists, audits, focused reports. | **Validation evidence.** May imply open follow-ups (track in needs-clarification/plan as needed). |
| **Progress notes** | [`progress/`](./progress/) | Dated progress tracking for specific workstreams. | Supporting progress evidence; actionable follow-ups should be reflected in plan/ready/needs-clarification. |
| **Review findings** | [`review-findings/`](./review-findings/) | Audits/reviews and findings. | Evidence/findings; convert actionable work into clarified items when appropriate. |

## Ready contract (state transition)

An item in `plan/ready/` must document: **ID/title**, **status `ready` (approved)**, **source/links**, **approval** (who/when/context, after specification), **open questions: none**, **scope/non-goals**, **acceptance criteria/DoD**, **validation plan** (automated per `AGENTS.md`; hardware/manual only when the deliverable is the guide/script or explicitly required and feasible), **expected areas/files**, **dependencies/prerequisites**, and **risks/constraints**. Items missing any of these, or blocked by unavailable decisions/hardware/external dependencies, belong in `plan/needs-clarification/`.

## Quick guide

- **Want the next implementable, pre-approved work?** Check [`plan/ready/`](./plan/ready/) first, then see [`plan/consolidated-improvement-plan.md`](./plan/consolidated-improvement-plan.md).
- **Want to clarify/open questions or create a spec?** Use [`plan/needs-clarification/`](./plan/needs-clarification/) with the `clarify-feature` skill / `/clarify-feature` (and `/new-feature` to create a standardized item).
- **Want the current overall picture?** Start with [`plan/consolidated-improvement-plan.md`](./plan/consolidated-improvement-plan.md).
- **Want detailed/historical context or evidence?** See [`plan/`](./plan/), [`history/`](./history/), [`ideas/`](./ideas/), [`reports/`](./reports/), [`progress/`](./progress/), and [`review-findings/`](./review-findings/).

## Maintenance rules

- **No data loss:** keep historical docs and reports; do not delete evidence.
- **Process all doc markdown files** when assessing pending/actionable work (not only the consolidated plan). Keep the consolidated plan linked/consistent as the canonical status tracker.
- **State transitions:** clarify and record approval after specification before moving an item to `plan/ready/`; move or keep items with open questions/blockers in `plan/needs-clarification/`.
- **Update status/evidence** when work lands; link validation evidence and cross-link ready/needs-clarification items to source docs and the consolidated plan.
- **Prefer stable links:** when adding new docs, place them in the appropriate area and cross-link as needed.
