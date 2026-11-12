---
name: implement-next-feature
description: Pick and implement the next sensible, complete feature or task from docs/. Use when asked to continue planned work, implement the next item, or work from docs/plan, docs/ideas, or docs/history with tracked progress and full validation.
---

# Implement Next Ongoing Feature

Use this skill to select and deliver one complete, well-scoped item from the repository documentation.

## 1) Explore docs and propose the next item

1. Read `docs/README.md` to understand doc structure and status meaning.
2. Review candidates in this order:
   - `docs/plan/consolidated-improvement-plan.md` (canonical tracker for current/planned work and follow-ups)
   - `docs/plan/*.md` (other active plans)
   - `docs/ideas/*.md` (draft proposals; only pick if self-contained and sensible to implement now)
   - `docs/history/*.md` and `docs/reports/*.md` for context/evidence on related items
3. Pick **one complete item** that makes sense to implement now. Prefer items that are:
   - Clearly scoped with acceptance criteria or an obvious definition of done
   - Not blocked by unavailable hardware, external decisions, or missing prerequisites (unless the deliverable is the preparation/guide/script itself)
   - High value relative to effort and consistent with current repo priorities/status
4. Report what you picked and why it is the right next item. Include:
   - Source document(s) and item identifier/heading
   - Scope and non-goals for this iteration
   - Expected files/areas to change
   - Validation plan (automated checks, integration tests, and whether on-device validation is required)
   - Any assumptions, risks, or blockers
5. **Stop and wait for explicit user approval before implementing.** Do not start implementation until approved.

## 2) Track progress in documents

After approval, keep progress visible in the relevant document(s):

- Update the source plan/idea/history item (or add a short status/progress note section if appropriate) with: status, what was implemented, validation run, evidence/links to reports or scripts, and any follow-ups.
- If the work came from an idea and becomes committed/active, reflect its status appropriately (and promote/link in `docs/plan/consolidated-improvement-plan.md` when that fits the repo's maintenance rules).
- Prefer stable, factual notes over speculative claims. Do not remove historical content; append progress/evidence.

## 3) Implement and validate

- Implement the approved scope with minimal, focused changes consistent with surrounding code and project conventions.
- Run relevant validation proactively (do not wait to be asked), fix failures, and re-run until passing. Follow `AGENTS.md` ("Definition of done").
- **Default checks (most changes):** `make format-check`, `make test`. If formatting fails, run `make format` then re-run `make format-check`.
- **Additional checks by scope** (run when applicable):
  - Frontend / `storage/ts` / `storage/flash_data`: from `frontend/`, `yarn format:check`, `yarn lint`, `yarn test`, `yarn build` (install deps with `yarn install --frozen-lockfile` if needed).
  - Firmware / build / lint-covered C/C++: `make build`, and `make lint` when feasible (requires ESP-IDF v6.1 environment sourced).
  - Documentation / Doxygen comments: `make docs-check`.
  - Integration-test-affected behavior: `make integration-test-build` and/or `make integration-test` / `make integration-test-run` (requires ESP-IDF v6.1, QEMU, and deps per README/CI).
- Report exactly which commands were run and pass/fail outcomes. If a required check cannot run, explain the concrete blocker and give actionable steps to resolve it (what to install/configure, versions, env vars/paths, and exact rerun command). Do not claim success for checks not executed.

## 4) Automate validation where possible

- Add or update **unit tests** and/or **integration tests** to cover the implemented behavior and regressions.
- Prefer automated, repeatable checks over manual verification. Keep tests aligned with existing test frameworks and locations in the repo.

## 5) On-device validation (only when required)

If the feature cannot be fully validated on the host/CI-like checks:

- Create a concise on-device validation guide (in the relevant doc/report location, e.g. `docs/reports/` or alongside the tracked item) describing prerequisites, steps, expected results, and how to capture evidence.
- If possible, create helper script(s) or console snippet(s) that can be executed on-device to minimize manual work. Document where they live and how to run them.
- Run everything that can be validated locally/automatically first, then clearly state what remains for hardware validation and ask for it.

## 6) Git and safety constraints

- **Do NOT perform Git write operations** (no `git commit`, `git push`, `git tag`, branch creation/deletion, staging for commit, or similar). Leave changes in the working tree for the user to review.
- Do not revert or overwrite unrelated user changes. Investigate unexpected modifications before editing.

## Deliverable checklist

- [ ] Proposed item reported with scope and validation plan; approval received before coding
- [ ] Progress tracked in relevant document(s) with evidence/follow-ups
- [ ] Implementation complete for approved scope
- [ ] Relevant automated tests added/updated where possible
- [ ] Required validation run (or blockers documented with actionable remediation)
- [ ] On-device guide/scripts/snippets provided if hardware validation is needed
- [ ] No Git write operations performed
