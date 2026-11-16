---
description: Create a new feature/item in the standard docs format
agent: build
---

Create a standardized feature/work item using the repository's state-based docs flow.

Request: $ARGUMENTS

- Determine a clear `<item-id>` (slug), title, and source/context from the request (ask for missing essentials only if necessary).
- Create `docs/plan/needs-clarification/<item-id>.md` with this structure (fill known details; leave unknowns as explicit open questions — do not invent requirements or approval):

```md
# <Title>

- **ID:** <item-id>
- **Status:** needs-clarification
- **Created:** <YYYY-MM-DD>
- **Source/links:** <origin/context; link consolidated plan item if applicable>
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
<what/why>

## Scope
- 

## Non-goals
- 

## Acceptance criteria
- [ ] 

## Validation plan
- Automated: (per `AGENTS.md` / affected scope)
- Hardware/manual: (only if required; otherwise "none")

## Expected areas/files
- 

## Dependencies / prerequisites
- 

## Risks / constraints
- 

## Open questions
- [ ] 

## Progress / evidence
- <YYYY-MM-DD>: created.
```

- If the item relates to tracked work, add/update links in `docs/plan/consolidated-improvement-plan.md` as appropriate (no history removal).
- Report the created path and remaining open questions, and suggest running `/clarify-feature <item-id>` to resolve them and promote to `docs/plan/ready/` when the Ready contract is satisfied and approved (and the item is implementable now). After implementation is complete with recorded evidence, move it to `docs/plan/done/`.
