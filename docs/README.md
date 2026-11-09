# COS Documentation

This directory is organized so you can quickly see what is **done/historical**, what is **planned/structured**, and what is still in **draft/idea** state.

## Structure

| Area | Path | Purpose | Status meaning |
|---|---|---|---|
| **Plan (living)** | [`plan/consolidated-improvement-plan.md`](./plan/consolidated-improvement-plan.md) | Cross-cutting tracker for priorities, ordering, acceptance criteria, progress, and follow-ups. | **Source of truth for current status** of planned/remaining work (R1–R24 and follow-ups). |
| **History / specs** | [`history/`](./history/) | Dated improvement plans, design specs, and conformance documents (2026-10 through 2026-11-08). | **Historical design/spec records.** Many items described here are delivered; some have follow-ups tracked in the consolidated plan. Treat these as background/evidence, not the live tracker. |
| **Ideas / drafts** | [`ideas/`](./ideas/) | New proposals that are not yet fully scheduled or accepted into the main execution plan. | **Draft/idea state.** Review, refine, and promote into the consolidated plan when ready. |
| **Reports / evidence** | [`reports/`](./reports/) | Hardware validation guides/results, profiling checklists, audits, and focused implementation reports. | **Validation evidence.** Supports plan items and historical specs; may include open follow-ups noted in the report/plan. |

## Quick guide

- **Want the current picture of what's left / planned?** Start with [`plan/consolidated-improvement-plan.md`](./plan/consolidated-improvement-plan.md).
- **Want design rationale or historical context for a delivered area?** Read the relevant file in [`history/`](./history/), then check the plan item status/evidence.
- **Want raw proposals / future possibilities?** Read [`ideas/2026-11-09-new-ideas.md`](./ideas/2026-11-09-new-ideas.md).
- **Want proof (tests, hardware runs, measurements, audits)?** See [`reports/`](./reports/).

## Current snapshot (2026-11-09)

The consolidated plan tracks delivery across shell/CI, Berry runtime memory/module loading, networking, TS→Berry compiler/SDK, Berry command scripts, and firmware execution contracts. Notable open/ongoing areas called out in the plan include:

- **R12** Milestone 4 hardware validation (touch/network) — pending hardware.
- **R14–R20** compiler profiles/debug/source maps, optional `.bec`/solidification experiments, UI allocation policy experiments, `async`/await runtime design, in-browser simulator, and 2026-10 deferred backlog — not started/backlog.
- **Follow-ups** such as HTTPS/PSA `-141` under heap pressure, timeout/abandon cleanup nuances, BT coexistence measurements, and stdio/heap corruption investigation (R5-FU1 context) — see the plan's progress/follow-up sections for exact status.

For details and evidence links, use the consolidated plan as the canonical tracker.

## Maintenance rules

- **No data loss:** keep historical docs and reports; do not delete evidence.
- **Update the plan** when status changes, and link validation evidence.
- **Promote ideas deliberately:** when an idea becomes committed work, add/refine it in [`plan/consolidated-improvement-plan.md`](./plan/consolidated-improvement-plan.md) (and keep the original idea doc as historical proposal context if useful).
- **Prefer stable links:** when adding new docs, place them in the appropriate folder and cross-link from the plan/report/README as needed.
