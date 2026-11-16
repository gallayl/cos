---
name: clarify-feature
description: Iteratively clarify a feature/item by asking focused questions until scope, acceptance criteria, validation, dependencies, and approval are explicit; record answers in docs and move the item to ready only when the Ready contract is satisfied.
---

# Clarify feature (grilling)

Use this skill to turn an ambiguous or questioned item into a fully specified, implementable work item. Primary goal: resolve open questions with the user and record decisions in markdown.

## State model

- **Needs clarification:** `docs/plan/needs-clarification/<item-id>.md` — item has open questions/blockers/unknowns or lacks required spec fields.
- **Ready (pre-approved queue):** `docs/plan/ready/<item-id>.md` — item meets the Ready contract, is approved, and is **ready to implement now** (no blockers). `plan/ready/` must contain only ready-to-implement items.
- **Done:** `docs/plan/done/<item-id>.md` — item implementation is complete and delivered with recorded evidence (moved out of `ready/` by implementation/progress tracking); pending merge/human or hardware validation may remain documented.
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
3. **Ask focused questions (prefer `question` tool, fall back to plain text).** Ask the smallest set of high-impact questions at a time (batch related ones).
   - **Preferred:** use the `question` tool (see **User interaction** below for required structure). Put the recommended option first with `(Recommended)` in its label and set `multiple: true` only when choices are not mutually exclusive.
   - **If `question` is unavailable, unsupported, or fails/errors:** do not retry it repeatedly; ask the same questions in plain chat as a numbered list. For each question include: context (1 line if needed), options labeled `A/B/C...` with short tradeoffs, and a clearly marked default/recommendation (e.g. `Recommended: A`). State that the user may reply with option letters, "default", or a free-form answer. Accept the user's reply as the answers for that round.
   - Do not guess or invent requirements/approvals.
4. **Record answers.** Update the item markdown after each round: capture decisions, resolved questions, remaining questions, and rationale. Keep notes factual; append progress (no history removal).
5. **Check readiness.** When no open questions remain and all Ready-contract fields are satisfied, ask for/record explicit approval (prefer `question` tool when available, otherwise plain text) per "ready/pre-approved asked after feature specification". Only then set status to ready and move the file to `docs/plan/ready/<item-id>.md` (or create it there) and update cross-links (including consolidated plan if applicable). If a ready item becomes blocked or gains open questions, move it back to `docs/plan/needs-clarification/` and update status/links.
6. **Stop when blocked.** If answers reveal external blockers (hardware-only, missing access/decision, out-of-scope dependency), keep the item in `needs-clarification` with blockers/questions clearly listed and what would unblock it. If no user answers are available in this interaction, record the open questions/blockers in the item and stop (do not invent answers).

## User interaction

When clarification or a user decision is required, prefer OpenCode's built-in `question` tool.

**IMPORTANT:** Follow the exact `question` tool argument structure.

Every `question` tool invocation MUST contain:

```json
{
  "questions": [
    {
      "header": "Short heading",
      "question": "Complete question text",
      "options": [
        {
          "label": "Option A",
          "description": "Description of option A"
        },
        {
          "label": "Option B",
          "description": "Description of option B"
        }
      ],
      "multiple": false
    }
  ]
}
```

Rules:
- Never invoke the `question` tool with an empty object.
- Always provide the top-level `questions` array.
- Every question must include `header`, `question`, and `options`.
- Keep questions explicit and self-contained; include concrete options with short tradeoff descriptions and put the recommended option first with `(Recommended)` in its label.
- Do not omit required fields, even when their meaning seems obvious from other fields.
- Set `multiple: true` only when choices are not mutually exclusive.
- If the `question` tool fails, ask the question directly in conversational text and wait for the user's response (see fallback in step 3).

## Output

- Updated markdown with current questions/decisions/status and links.
- Clear statement of: resolved, still open, and whether the item is now Ready (and moved) or remains in needs-clarification.
