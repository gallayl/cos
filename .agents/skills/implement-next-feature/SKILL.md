---
name: implement-next-feature
description: Pick and implement the next sensible, complete feature or task from docs/, then deliver it through a safe Git/PR flow. Use when asked to continue planned work, implement the next item, or work from docs/plan, docs/ideas, or docs/history with tracked progress, validation, branch/PR delivery, and CI follow-up.
---

# Implement Next Ongoing Feature

Use this skill to select and deliver one complete, well-scoped item from the repository documentation, with a controlled development flow (baseline check, feature branch, local validation, PR, and CI iteration). **This full Git/PR delivery flow is the default** whenever this skill is used.

**Autonomy model:** After selecting/proposing the work item, **wait for explicit user approval ("go")** before starting implementation and Git/PR operations. Once approved, work autonomously until the feature is ready, the branch is up to date, and the pipeline is green. Do not pause for per-step approvals (branching, committing, pushing, PR creation/updates). The only acceptable interruptions after approval are (a) clarifications needed to resolve ambiguity discovered during execution and (b) hard blockers that cannot be resolved safely/autonomously (e.g. network/auth/permission failures, cannot push, missing required access/tooling with no viable fallback, or a product/infra decision requiring human input). Never merge PRs automatically.

## 0) Preflight and baseline

Before proposing or changing code, establish a safe baseline:

1. Inspect repository state: current branch, `git status`, remotes, and whether the working tree is clean. Do not discard, revert, or overwrite unrelated user changes; investigate and report unexpected modifications.
2. Identify the default integration branch (e.g. `main`/`master`) and confirm the local and remote CI health for that baseline when possible. Prefer GitHub CLI when available (e.g. `gh run list --branch <base>`, `gh run view`, check status on the branch/HEAD), and report concrete evidence. If CI/health cannot be determined (tooling/auth/network), report the blocker and ask how to proceed.
3. **If the baseline is red/failing, fix the baseline first.** A green baseline is a must. Treat baseline restoration as the priority work item: handle it on a **separate baseline-fix branch/PR** (not mixed into unrelated feature work), drive it to green, open the PR, and **pause for the baseline-fix PR to be merged** (human/maintainer action) before starting feature work. Do not build unrelated feature changes on top of a known-broken or unmerged baseline fix. After merge, re-sync the healthy base and re-run preflight/proposal as needed.
4. Sync local refs (`git fetch`, and update the local default branch if appropriate and safe). Do not perform destructive operations (`reset --hard`, forced pulls, rebases over unknown work).
5. Check whether dependency/tooling updates are actually required for the chosen scope (lockfiles, environment, CI/tool versions). **Never auto-upgrade dependencies** unless the selected work item is explicitly about dependency updates or the user explicitly requests it. If updates seem necessary, use project-pinned mechanisms and explain what/why in the proposal/docs/PR; only escalate as a blocker if the change is risky/ambiguous or cannot be completed safely. Routine pinned installs/syncs needed to run checks (e.g. `yarn install --frozen-lockfile` per project guidance) are fine when required.
6. Confirm required tooling/auth for the intended flow (git remote access, **GitHub CLI `gh` preferred** for PR/CI inspection/creation). If unavailable, continue with local-only steps where possible and clearly state what remains, with exact fallback commands/instructions.

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
   - Compatible with a healthy baseline (or explicitly includes the baseline fix/prep as in-scope)
4. Report what you picked and why it is the right next item. Include:
   - Source document(s) and item identifier/heading
   - Scope and non-goals for this iteration
   - Expected files/areas to change
   - Validation plan (automated checks, integration tests, and whether on-device validation is required)
   - Git/PR plan (base branch, proposed feature/baseline-fix branch name, commit/PR intent) and any dependency/tooling changes that may be needed
   - Assumptions, risks, blockers, and baseline/CI status
5. **Stop and wait for explicit user approval ("go") before implementing or performing Git write operations.** Ask the user if clarification is needed to choose or scope the right work item. Once approval is given, proceed autonomously with the agreed plan. If baseline restoration is required, handle it as a separate baseline-fix branch/PR first and pause for that PR to be merged before starting feature work from a healthy base.

## 2) Prepare a working branch

- Create/switch to a dedicated branch from the agreed, up-to-date, healthy base (a feature branch for feature work, or a **separate baseline-fix branch** when restoring baseline health). Ensure the working tree state is understood and avoid carrying unrelated local changes.
- Use a clear branch name aligned with repo conventions (or a sensible default if none exist).
- If the base moves or the branch becomes stale later, update it by **merging** the base branch into the working branch (preferred) and re-run relevant validation. Avoid rebase/force-push unless explicitly required and approved.

## 3) Track progress in documents

Keep progress visible in the relevant document(s) throughout the work:

- Update the source plan/idea/history item (or add a short status/progress note section if appropriate) with: status, what was implemented, validation run, evidence/links to reports or scripts, and any follow-ups.
- If the work came from an idea and becomes committed/active, reflect its status appropriately (and promote/link in `docs/plan/consolidated-improvement-plan.md` when that fits the repo's maintenance rules).
- Keep docs and the eventual PR description consistent (scope, validation, remaining hardware steps, known limitations).
- Prefer stable, factual notes over speculative claims. Do not remove historical content; append progress/evidence.

## 4) Implement and validate locally

- Implement the approved scope with minimal, focused changes consistent with surrounding code and project conventions.
- Run relevant validation proactively (do not wait to be asked), fix failures, and re-run until passing. Follow `AGENTS.md` ("Definition of done").
- **Default checks (most changes):** `make format-check`, `make test`. If formatting fails, run `make format` then re-run `make format-check`.
- **Additional checks by scope** (run when applicable):
  - Frontend / `storage/ts` / `storage/flash_data`: from `frontend/`, `yarn format:check`, `yarn lint`, `yarn test`, `yarn build` (install deps with `yarn install --frozen-lockfile` if needed).
  - Firmware / build / lint-covered C/C++: `make build`, and `make lint` when feasible (requires ESP-IDF v6.1 environment sourced).
  - Documentation / Doxygen comments: `make docs-check`.
  - Integration-test-affected behavior: `make integration-test-build` and/or `make integration-test` / `make integration-test-run` (requires ESP-IDF v6.1, QEMU, and deps per README/CI).
- Report exactly which commands were run and pass/fail outcomes. If a required check cannot run, explain the concrete blocker and give actionable steps to resolve it (what to install/configure, versions, env vars/paths, and exact rerun command). Do not claim success for checks not executed.
- Ensure local validation is in a good state before committing/pushing (or clearly document any environment-limited gaps).

## 5) Automate validation where possible

- Add or update **unit tests** and/or **integration tests** to cover the implemented behavior and regressions.
- Prefer automated, repeatable checks over manual verification. Keep tests aligned with existing test frameworks and locations in the repo.

## 6) On-device validation (only when required)

If the feature cannot be fully validated on the host/CI-like checks:

- Create a concise on-device validation guide (in the relevant doc/report location, e.g. `docs/reports/` or alongside the tracked item) describing prerequisites, steps, expected results, and how to capture evidence.
- If possible, create helper script(s) or console snippet(s) that can be executed on-device to minimize manual work. Document where they live and how to run them.
- Run everything that can be validated locally/automatically first, then clearly state what remains for hardware validation and ask for it.

## 7) Commit, push, and open a pull request

Proceed with these Git write/remote operations as part of the default autonomous flow when safe and credentials/tooling are available (stop only for hard blockers):

- Stage and commit focused changes with clear **Conventional Commits** messages describing the scoped work (e.g. `feat:`, `fix:`, `docs:`, `test:`, `chore:` as appropriate; include scope when useful). Do not include unrelated files. **Multiple commits are fine** when they represent logical steps; maintainers may squash-merge.
- Push the working branch to the remote.
- Open a pull request against the agreed base branch. Prefer **GitHub CLI (`gh pr create`)** when available. Prefer opening as a **draft PR** while work/CI is in progress when supported; mark ready when the branch is up to date and checks are green. (In this repo, `.github/workflows/*` trigger on `pull_request` to `main` without `types` filters, so default PR events—including draft PR open/synchronize—should trigger the configured pipelines; `ready_for_review` is not required by current workflow configuration. Confirm actual check runs and adjust if a workflow is configured differently.)
- Keep the PR description **minimal**: short summary/scope and **links to the relevant documentation** (plan/idea/history/report items) that contain detailed progress, validation evidence, and follow-ups. Also call out any required hardware/manual validation steps and known limitations/blockers when applicable.
- If push/PR creation or CI visibility is blocked (permissions, auth, missing `gh`, network), report the blocker with evidence and provide exact commands/instructions for the user to complete the step.

## 8) Iterate on CI and review feedback

After the PR is open, drive the change to a healthy state autonomously:

- Check that the working branch is up to date with the base branch; if not, sync it by **merging** the base branch into the working branch (preferred), resolve conflicts carefully, and re-run relevant local validation.
- Monitor CI/check status for the PR/branch (prefer `gh pr checks`, `gh run list/view`, or equivalent). Poll/check as needed and report progress and next actions; do not assume continuous unattended monitoring. When checks fail, inspect logs/output, identify the root cause, implement the fix, update tests/docs and the PR description/links if behavior or validation changed, re-run local checks, and push updates (using Conventional Commits as appropriate).
- Repeat until checks are green and the branch is up to date, or a clear external/hard blocker remains. Do not guess; base fixes on evidence from logs, failing tests, and reproduced behavior. Avoid endless churn—if progress is blocked or requires a product/infra decision, report status and ask for guidance.
- **Do not merge the pull request.** Once automated checks are green, the branch is up to date, and required artifacts/docs/guides are in place, inform the developer of the final state and explicitly call out any hardware/on-device validation still required. The developer/maintainer performs the merge (squash-merge expected; commit history on the branch may contain multiple commits).

## 9) Git and safety constraints

- This Git/PR flow is the **default**. **Wait for explicit user approval ("go")** after proposing the work item before performing implementation and Git write/remote operations. Once approved, run autonomously (branch creation, commits, push, PR creation/updates) until ready/up-to-date/green, pausing only for necessary clarifications discovered during execution or hard blockers. Never merge PRs.
- **A green baseline is a must.** Baseline fixes must go on a **separate branch/PR** from unrelated feature work; pause for the baseline-fix PR to be merged before starting feature work.
- **Never auto-upgrade dependencies** unless the selected work item is explicitly about dependency updates or the user explicitly requests it.
- Do not amend/rewrite shared history, force-push, delete branches, or run destructive commands. Prefer merging base into the working branch to keep it up to date.
- Do not revert or overwrite unrelated user changes. Investigate unexpected modifications before editing.
- Respect `AGENTS.md` and project conventions at all times. Use `gh` CLI as the preferred tool for PR/CI operations when available.

## Deliverable checklist

- [ ] Baseline checked (repo state, base branch, CI/remote health as available); if red, baseline fixed via separate branch/PR and merged before feature work
- [ ] Work item reported with scope, validation plan, and Git/PR plan; explicit user approval ("go") received before implementation/Git writes
- [ ] Working branch prepared from agreed healthy base (separate baseline-fix or feature branch as appropriate)
- [ ] Progress tracked in relevant document(s) with evidence/follow-ups (and linked from PR)
- [ ] Implementation complete for approved scope
- [ ] Relevant automated tests added/updated where possible
- [ ] Required local validation run (or blockers documented with actionable remediation)
- [ ] On-device guide/scripts/snippets provided if hardware validation is needed
- [ ] Changes committed (Conventional Commits), pushed, and PR opened/updated (prefer draft while iterating) with minimal description linking to docs
- [ ] Branch kept up to date (merge from base) and CI/review feedback iterated to green, or clearly blocked with evidence
- [ ] Developer informed of final state, remaining hardware validation (if any), and that merge (squash-merge) is a human action
