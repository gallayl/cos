---
name: track-doc-progress
description: Granular procedure to record implementation progress, validation evidence, and follow-ups in the relevant docs and keep them consistent with PR scope. Use when composing docs-driven or planned work.
---

# Track doc progress (granular)

Keep progress visible and factual in repository documentation. **Granular skill** composed by primary/repo-specific workflows.

## Workflow

1. **Identify source docs:** the plan/idea/history/report item(s) that define or track the work (see `docs/README.md` and `docs/plan/consolidated-improvement-plan.md` as canonical tracker when applicable).
2. **Update status/progress** in the relevant document(s): what was implemented, scope/non-goals, validation run (exact checks/outcomes or blockers), evidence/links to reports or scripts, and follow-ups/remaining steps.
3. **Maintain queue state:** keep `docs/plan/ready/` for items that are still ready-to-implement now. If a ready item becomes blocked/not ready-to-implement, move it to `docs/plan/needs-clarification/`. If the scoped implementation is complete and delivered (PR opened/updated, required automated checks/PR CI green per scope, and validation/evidence recorded), move the item from `docs/plan/ready/` to `docs/plan/done/` and update status/cross-links (including consolidated plan if applicable) in the same change. Record any remaining human actions in the item: review/approval and merge remain human, and required hardware/manual validation must be stated explicitly (and surfaced as a blocking PR comment/thread until resolved with evidence or explicitly waived). Do not claim unrun checks or hardware validation. Moving to `done/` does not imply merge.
4. **Maintain consistency:** keep docs aligned with the eventual/current PR description (scope, validation, hardware/manual steps, known limitations). When an idea becomes active/committed, reflect status appropriately and link/promote in the consolidated plan when it fits repo maintenance rules.
5. **Quality:** prefer stable, factual notes over speculation. **Do not remove historical content;** append progress/evidence.

## Output

- List documents updated/moved and a brief summary of recorded status/evidence.
