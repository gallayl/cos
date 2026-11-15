---
description: Clarify an item until it is fully specified and ready
agent: build
---

Follow the `clarify-feature` skill, root `AGENTS.md`, and `docs/README.md`.

Target item: $ARGUMENTS

- Locate or create the item in `docs/plan/needs-clarification/<item-id>.md` (if a source doc/path/ID is given, use it; ask only if the target cannot be identified).
- Review relevant docs/code context, identify open questions and spec gaps, and ask the user focused, concrete questions until the Ready contract is satisfied or a blocker remains. Prefer the `question` tool and follow its exact argument structure/rules in the skill (`questions` array; each question has `header`, `question`, `options`; never empty object). If unavailable or it fails, ask in plain chat as a numbered list with labeled options and a recommended default, and do not retry the tool repeatedly.
- Record decisions/answers and remaining questions in markdown after each round (append factual notes; no history removal). Keep `docs/plan/consolidated-improvement-plan.md` links/status consistent when applicable.
- When (and only when) there are no open questions, all Ready-contract fields are complete, and explicit approval is given/recorded after specification, mark the item ready and move it to `docs/plan/ready/<item-id>.md`.
- If blocked, unanswered, or no user response is available, keep it in `docs/plan/needs-clarification/` with clear questions/blockers and what would make it ready. Do not invent requirements or approvals.
