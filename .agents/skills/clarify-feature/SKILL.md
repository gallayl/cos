---
name: clarify-feature
description: Iteratively clarify a feature/item by asking focused questions until scope, acceptance criteria, validation, dependencies, and approval are explicit; record answers in docs and move the item to ready only when the Ready contract is satisfied.
---

# Clarify feature (grilling)

Use this skill to turn an ambiguous or questioned item into a fully specified, implementable work item. Primary goal: resolve open questions with the user and record decisions in markdown.

## State model

- **Needs clarification:** `docs/plan/needs-clarification/<item-id>.md` — item has open questions/blockers/unknowns or lacks required spec fields.
- **Ready (pre-approved queue):** `docs/plan/ready/<item-id>.md` — item meets the Ready contract and is cleared to implement.
- Canonical status/evidence remains tracked in `docs/plan/consolidated-improvement-plan.md` when applicable (link both ways; do not remove historical content).

## Ready contract (all required)

An item is Ready only when documented with:
- **ID/title**, **status:** `ready` (approved), **source/links** (originating plan/idea/history/report and consolidated-plan item if any).
- **Approval:** who approved, when/context, and explicit pre-approval for implementation/PR flow if intended.
- **Open questions:** `none` (explicit).
- **Scope and non-goals**.
- **Acceptance criteria / definition of done**.
- **Validation plan** (automated checks per `AGENTS.md`; required on-device/hardware steps only if the deliverable itself is the guide/script — otherwise not autonomously “implementable right away”).
- **Expected areas/files** (rough is fine), **dependencies/prerequisites**, and **risks/constraints**.

If any field is missing, ambiguous, or blocked by an unavailable decision/hardware/external dependency, the item is **not** Ready.

## Workflow (grilling loop)

1. **Load context.** Read the target item (`docs/plan/needs-clarification/<item-id>.md` or source doc), related docs (`docs/README.md`, consolidated plan, linked plans/ideas/reports/history), and relevant code areas if needed to ask informed questions.
2. **Identify gaps.** List concrete open questions/blockers (scope boundaries, behavior/contract, UX/output, API/schema, compatibility, rollout, validation, dependencies, ownership, approvals).
3. **Ask focused questions.** Ask the smallest set of high-impact questions at a time. Prefer specific options/tradeoffs and examples; do not guess or invent requirements/approvals.
4. **Record answers.** Update the item markdown after each round: capture decisions, resolved questions, remaining questions, and rationale. Keep notes factual; append progress (no history removal).
5. **Check readiness.** When no open questions remain and all Ready-contract fields are satisfied, ask for/record explicit approval (per "ready/pre-approved asked after feature specification"). Only then set status to ready and move the file to `docs/plan/ready/<item-id>.md` (or create it there) and update cross-links (including consolidated plan if applicable).
6. **Stop when blocked.** If answers reveal external blockers (hardware-only, missing access/decision, out-of-scope dependency), keep the item in `needs-clarification` with blockers/questions clearly listed and what would unblock it.

## Output

- Updated markdown with current questions/decisions/status and links.
- Clear statement of: resolved, still open, and whether the item is now Ready (and moved) or remains in needs-clarification.
