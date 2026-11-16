---
description: Implement the next ready/planned docs feature (composed workflow)
agent: build
---

Follow the `implement-next-feature` skill (repo-specific, composed) and root `AGENTS.md`.

Target/request: $ARGUMENTS

- Run the full composed flow: `ensure-green-baseline` → `pick-next-work-item` → (gate per Ready contract/pre-approval rules) → `prepare-working-branch` → `track-doc-progress` → implement/test/validate (`implementation-workflow`, `testing`/`validate-change`, `add-device-script-example` as applicable) → independent `reviewer`/`code-review` for substantive changes → `deliver-pr` → **immediately** `babysit-pr` in the same run until merge-ready or hard-blocked → finalize doc state → report final state.
- Preserve gates: ready-compliant `docs/plan/ready/` items are pre-approved for implementation/PR/babysit-to-green (merge remains human); require explicit `"go"` only when the workflow/item requires it; green baseline, safe Git (no force-push/history rewrites, never merge PRs), and evidence-based validation/reporting (never claim unrun checks).
- Do not stop at PR creation; do not treat PR creation as completion. Report actions, commits/PR links, exact checks run and outcomes, review findings/resolution, and remaining blockers/human actions.
