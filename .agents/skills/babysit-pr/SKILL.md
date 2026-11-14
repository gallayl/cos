---
name: babysit-pr
description: Babysit a GitHub pull request until it is merge-ready - keep it up to date, get all PR GitHub Actions green, and resolve review feedback by validating, fixing, or replying with a clear rationale.
---

# Babysit Pull Request

Use this skill to drive a GitHub pull request to a merge-ready state. Work autonomously once the target PR is identified; stop only on hard blockers (missing auth/tooling, merge conflicts that cannot be resolved safely, or ambiguity that needs a human decision). **Never merge the PR.**

## 1) Identify the PR

- If the user provides a PR number or URL, use it.
- Otherwise, if a branch is checked out, find its corresponding open PR (e.g. `gh pr view --json number,url,headRefName,baseRefName,state` or `gh pr list --head <branch>`).
- **If no PR can be associated, stop and state that the PR cannot be associated.** Report the current branch and what you checked.

Gather baseline context: PR number/URL, head/base branches, draft/review state, mergeability, checks, and all discussion surfaces (review threads, "changes requested" reviews, and general PR conversation). Prefer `gh` for PR/CI operations.

## 2) Keep the branch up to date (merge, simple Git)

- Sync refs (`git fetch`). If the PR branch is behind or not mergeable with the base, **update it by merging the base branch into the PR branch** (not rebase-by-default workflow changes; keep Git simple).
- You may pull, commit, push, and merge/rebase locally only as needed for a straightforward update, but **do not use force-push or other force operations**. Do not rewrite shared history.
- If merge conflicts arise, attempt a careful, minimal resolution consistent with the intended change. If conflicts are risky/unclear or cannot be resolved safely, stop and report the conflicting files and what decision is needed.
- After updating, push the branch and re-run/re-check relevant local validation when code changed.
- Keep the **PR description up to date** with the current scope, key changes, validation status/notes, and any remaining manual steps or blockers (update via `gh pr edit` or equivalent) so it accurately reflects the PR as-is.

## 3) Babysit CI (all GitHub Actions on the PR)

- **All GitHub Actions checks on the pull request must pass.** Monitor every PR check/run (e.g. `gh pr checks`, `gh run list/view`).
- If any check is **in progress or queued, poll** until it completes. Determine polling/wait limits from the workflow definitions in `.github/workflows/*.yml`/`*.yaml` (use `timeout-minutes` at job/step/workflow level where present; apply a sensible overall cap if unspecified). Report progress while waiting; do not assume unattended monitoring.
- Treat **skipped/not-applicable** checks according to GitHub/PR status (a check that is legitimately skipped by workflow conditions and not required as a failure does not block), but investigate unexpected skips.
- When checks fail, inspect logs, identify the root cause, implement the minimal fix, update tests/docs if behavior or validation changed, run relevant local validation, commit (Conventional Commits as appropriate), and push. Repeat until checks are green or a hard blocker remains (base fixes on evidence, not guesses; avoid churn).

## 4) Resolve review feedback (validate, then fix or reply)

Handle **all** of: unresolved review threads, reviews requesting changes, and general PR comments. Do not ignore bot comments.

For each item:

1. **Validate** it: read the code and surrounding context, understand intent/history, and reproduce or run relevant checks/tests when applicable. Base conclusions on evidence.
2. **If valid/actionable:** make the minimal fix, run relevant validation, commit and push, then **reply explaining what changed and reference the fixing commit** (e.g. "Fixed in <sha>: <one-line summary>"). Resolve the thread **after** replying, when appropriate.
3. **If invalid/misunderstood/not justified:** **reply with a compact, straight-to-the-point explanation** of why (cite code/behavior/evidence as needed) and then resolve. Do **not** resolve a thread without replying. Only resolve-and-reply-without-code-change when the finding is genuinely invalid, misunderstood, out of scope, intentional, or cannot be justified.

Keep replies compact and factual (no fluff, no required tone). Do not dismiss actionable feedback.

## 5) Definition of done

Babysitting is complete when:

- The PR branch is up to date with the base (merged as needed).
- The **PR description is up to date** and accurately reflects the current state.
- **All GitHub Actions checks on the PR are green** (or legitimately non-blocking/skipped per workflow).
- All review threads, change requests, and relevant discussion points are resolved/addressed per above.
- The PR is ready to merge. A pending human **review/approval** may remain; call it out explicitly.

## 6) Final report

Report **actions taken** and anything **not obvious from the default PR screen**. Do not list routine CI/test executions in detail. Include, as applicable:

- PR identified (number/URL/branch) and merge/update actions performed (including conflict resolutions or blockers).
- Commits pushed (SHAs + brief purpose) and files/areas changed to address feedback or CI.
- Each review/comment item handled: what was done, reply posted, and whether resolved (with brief rationale for invalid/not-actionable cases).
- Remaining blockers or required human actions (e.g. "review/approval needed", conflicts needing decision, auth/tooling/CI limits, environment-only validation).

## Safety

- Keep Git simple and non-destructive: no force-push/force operations, no history rewrites, no discarding unrelated user changes.
- Follow repository/project instructions (e.g. `AGENTS.md`) and run relevant local validation for changes you make. Report exact commands and outcomes when you run them.
- Prefer `gh` for GitHub interactions; if unavailable, explain the blocker and provide exact fallback steps.
