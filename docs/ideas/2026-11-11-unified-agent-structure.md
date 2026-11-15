# Cross-IDE / Agent Harness Configuration Strategy

## Goal

Maintain one Git-native source of truth for AI coding instructions, rules, skills, and workflows that works across Cursor, OpenCode, VS Code Copilot, and future agent harnesses.

Build toward a reusable, portable autonomous development workflow (inspired by the Ralph Wiggum / continuous-loop approach) with support for planning, implementation, testing, independent review, and automatic continuation—while keeping the architecture simple and evidence-based.

**Core principle:** Standardize shared content, use native tool-specific configurations only when necessary.

## Decisions (recorded)

1. **Skill layering:** Use **portable, high-level skills as the primary entry procedures** (e.g. implementation, investigation, testing, review). They **compose** smaller repo-specific/granular skills where reuse or repo-specific gates are needed. Avoid duplicate or competing sources of truth for the same flow.
2. **`/implement` semantics:** `/implement` is the **generic implementation workflow** (not tied to “pick the next feature from `docs/`”). Repo-specific flows (e.g. docs-driven next-work selection, baseline-fix PR gating, doc-progress tracking, PR delivery conventions) are handled by composing the relevant granular skills/procedures under that generic workflow or via separate, explicitly named entry points.
3. **Harness support:** Keep **Cursor, OpenCode, and VS Code Copilot** supported. Avoid duplication and stick to shared standards where possible; merge `.cursor/rules` content into the standard shared locations (`AGENTS.md`, nested `AGENTS.md`, `.agents/skills/`) rather than keeping parallel rule sources.
4. **Loop timing:** The autonomous loop/orchestrator plugin is **deferred until the minimal non-loop architecture is validated** (shared foundation + layered skills + thin commands/roles). Treat it as an optional later phase.
5. **OpenCode adapter form & permissions:** Prefer **file-based definitions** for OpenCode (`.opencode/agents/*.md`, `.opencode/commands/*.md`) over config-only entries where practical. Apply least-privilege defaults (see §4): reviewer/architect read-only (no implementation edits), test-runner/validator scoped shell/test execution with evidence-based reporting, subagent delegation explicitly allowed/denied per parent, and autonomous side effects limited (no auto-commit/push/deploy/publish by default).

## 1. Building Blocks

Use these building blocks with clear separation of responsibilities:

| Component            | Standard / Location         | Responsibility |
| -------------------- | --------------------------- | -------------- |
| Project instructions | `AGENTS.md` (+ nested)      | Shared project rules, conventions, architecture, DoD |
| Skills               | `.agents/skills/`           | Reusable workflows/procedures (portable, layered/composed) |
| Commands             | Tool-specific (thin)        | Explicit user-invocable workflow entry points |
| Custom agents        | Tool-specific (thin)        | Specialized roles, models, tools, permissions |
| Subagents            | Tool-specific agent mode    | Delegated workers with bounded context/permissions |
| Loop orchestrator    | Tool-specific plugin/runtime| Iteration/continuation/completion detection (deferred, optional) |
| Hooks / MCP / config | Tool-specific configuration | Runtime integration, permissions/policies, external tools |

### Shared foundation

**`AGENTS.md`**

- Architecture and repository structure.
- Coding conventions and standards.
- Technology choices and constraints.
- Build, test, lint, formatting, and definition-of-done commands (aligned with CI).
- Invariants that must always hold (e.g. cross-cutting API/binding parity, doc/check requirements).
- References to relevant skills and workflows.

**Nested `AGENTS.md`**

- Use for path-scoped guidance that applies whenever work touches that area (e.g. `frontend/AGENTS.md` for UI/framework conventions). Prefer nested instructions over duplicating scoped tool rules.

**Agent Skills (`.agents/skills/`)**

- Use the [Agent Skills specification](https://agentskills.io/).
- Each skill has a `SKILL.md` with YAML frontmatter.
- **Layering:** high-level, portable skills are the primary entry procedures; they may compose smaller granular skills (including repo-specific ones) for reusable sub-procedures and gates. Keep each skill focused and avoid duplicating logic across layers.
- Store supporting documentation, scripts, and references alongside the skill.
- Avoid duplicating general project rules inside skills—reference `AGENTS.md`/nested instructions instead.

## 2. Recommended Repository Structure

```text
repository/
│
├── AGENTS.md
├── frontend/
│   └── AGENTS.md                     # path-scoped shared guidance (if needed)
│
├── .agents/
│   └── skills/                       # portable, layered, reusable workflows
│       ├── implementation-workflow/  # primary (generic) entry procedure
│       │   └── SKILL.md
│       ├── bug-investigation/        # primary entry procedure
│       │   └── SKILL.md
│       ├── testing/                  # primary entry procedure
│       │   └── SKILL.md
│       ├── code-review/              # primary entry procedure
│       │   └── SKILL.md
│       ├── pick-next-work-item/      # granular/repo-specific (composed)
│       ├── ensure-green-baseline/    # granular/repo-specific (composed)
│       ├── prepare-working-branch/   # granular (composed)
│       ├── track-doc-progress/       # granular/repo-specific (composed)
│       ├── validate-change/          # granular (composed)
│       ├── deliver-pr/               # granular (composed)
│       └── babysit-pr/               # granular/reusable (composed)
│
├── .cursor/                          # Cursor adapter (thin, only if/when needed)
│   └── agents/
│
├── .opencode/                        # OpenCode adapter (thin, only if/when needed)
│   ├── agents/                       # preferred: file-based agent definitions
│   │   ├── architect.md
│   │   ├── reviewer.md
│   │   └── test-runner.md
│   └── commands/                     # preferred: file-based command definitions
│       ├── implement.md              # generic implementation workflow
│       ├── investigate.md
│       └── verify.md
│       # loop.md / cancel-loop.md and plugins/agent-loop/ deferred (see §7, §9)
│
└── .github/                          # Copilot adapter (thin, only if/when needed)
    ├── agents/
    └── instructions/
```

Do not create directories or adapter files until there is an actual need for them. Keep shared instructions in `AGENTS.md` (+ nested), portable/layered skills in `.agents/skills/`, and harness-specific agents/commands in their native adapter directories. Avoid duplicated instructions and unnecessary synchronization scripts. `.cursor/rules` content is to be migrated into the standard shared locations (not kept as a parallel rule source).

## 3. Separation of Responsibilities

Keep the concepts clearly separated:

- **Rules / instructions:** What agents must consistently know or obey (durable constraints, conventions, DoD). Live in `AGENTS.md` / nested `AGENTS.md`.
- **Skills:** How to perform specific, reusable tasks/workflows. Live in `.agents/skills/`; **high-level skills are primary entry procedures** and compose granular skills where appropriate.
- **Commands:** Explicit, user-invocable entry points (“do this workflow now”). Keep them **thin** (harness-specific): they select an agent/mode/model and invoke or reference shared skills/workflows rather than embedding full procedure text.
- **Agents:** Named roles with particular responsibilities, tools, permissions, and model/config. Keep them **thin** and tool-native; they reference shared instructions/skills. Agents make decisions and perform work.
- **Subagents:** Delegated workers that run in a bounded child context/session to handle a scoped task (e.g. exploration, validation, review, babysitting). Use them for real parallelism or permission/context boundaries—not as ceremony for every linear step.
- **Loop orchestrator:** Manages execution state and continuation (iterations, progress/completion detection, cancellation, safety limits). It submits continuation prompts and persists loop state; it does **not** replace evidence-based verification. **Deferred** until the minimal non-loop architecture is validated.
- **Hooks:** Automation triggered by lifecycle events (tool-native).
- **MCP / runtime config:** External tools, providers, permissions/policies, and other execution configuration (tool-native).

Non-instruction runtime configuration (hooks, MCP servers, provider/policy/permission enforcement, plugins, etc.) stays in tool-native config/adapters; do not force it into `AGENTS.md` or skills.

**Core architectural principle:** The loop controls execution, the orchestrator/primary agent controls decisions, agents perform specialized work, skills define procedures, and automated tests/results determine whether the task is actually finished.

## 4. Commands, Agents, and Subagents: Native Adapters

Command, agent, subagent, and (later) loop definitions are not fully standardized because harnesses expose different capabilities. Keep shared workflow logic in `.agents/skills/` and `AGENTS.md`; use native definitions as thin adapters. Support **Cursor, OpenCode, and VS Code Copilot** with minimal, non-duplicative adapters.

### OpenCode (follow OpenCode implementation rules)

- **Preferred form:** **File-based definitions** — agents in `.opencode/agents/<name>.md`, commands in `.opencode/commands/<name>.md`. Use `opencode.json(c)` (validated against `https://opencode.ai/config.json`) for server/project settings and only where config-based agent/command definition is clearly preferable. CLI/TUI preferences live separately in `cli.json` (not `opencode.json`).
- **Agents:** Markdown frontmatter + body (body becomes `system` prompt) with the same fields as config `agents`. Set `mode` deliberately:
  - `primary` — main session agent.
  - `subagent` — runs only as a child session via the `subagent` tool (fresh context; parent gets idle/result reports).
  - `all` — usable as primary or subagent.
  Key fields: `description` (important for subagent selection), `model` (`provider/model`, optional `#variant` where supported), `permissions` (ordered allow/ask/deny rules), `steps`, `hidden`, `disabled`, etc. Control which subagents a parent may launch with `subagent` permission rules on the parent.
- **Commands:** Markdown body = `template`; optional frontmatter: `description`, `agent`, `model`, `subagent`. Use `$ARGUMENTS`/positional args/`!`shell blocks as needed. Keep templates thin and delegate procedure to shared skills/instructions.
- **Subagents (usage):** A subagent is an agent with `mode: subagent` invoked through the `subagent` tool. Configure for bounded, delegable work with least-privilege permissions.
- **Suggested least-privilege defaults (apply per role):**
  - **Reviewer / Architect:** read-only analysis roles — deny `edit` (and `shell` unless explicitly required for read-only inspection) so they **must not modify implementation files**.
  - **Test Runner / Validator:** allow only the shell/test execution needed to run and report required checks; never claim unrun checks passed; report evidence and actionable blockers.
  - **Orchestrator/Implementer:** scoped read/write/shell as needed for the task; delegate specialized work to subagents where useful.
  - **Subagent delegation:** explicitly allow/deny `subagent` resources on parents (least privilege, only intended subagents).
  - **Autonomous side effects:** no auto-commit, no push, no deploy/publish by default; require explicit approval/policy per workflow (loop/plugin, if later added, must enforce this).
- **Loop/orchestrator plugin (deferred, OpenCode-specific):** If implemented later, keep it as a small OpenCode plugin (e.g. `.opencode/plugins/agent-loop/`) responsible for execution state/continuation only. Relevant APIs include `Plugin.define`, `ctx.command.transform()`, `ctx.event.subscribe()` (e.g. `session.idle`), `ctx.session.prompt()`, `ctx.session.context()`, and `ctx.storage`. It must not declare success without evidence-based completion criteria.

### Other harness adapters

| Tool            | Typical location            | Notes                                      |
| --------------- | --------------------------- | ------------------------------------------ |
| Cursor          | `.cursor/agents/` (and minimal adapter files only if required) | Migrate `.cursor/rules` to shared `AGENTS.md`/nested `AGENTS.md`/skills; keep Cursor-specific only when a capability cannot be expressed in shared standards |
| VS Code Copilot | `.github/agents/`, `.github/instructions/` (if needed) | Native agent/instruction forms as required |

Avoid duplicating complete agent, command, or loop instructions across harnesses. Define reusable workflows in `.agents/skills/`, reference them from native adapters, and keep tool-specific files limited to capabilities, permissions, models, and execution settings.

## 5. Agents, Skills, and Commands (Starter Set)

Start minimal: validate a simple architecture before building a complicated multi-agent framework. Run agents sequentially by default; use parallelism only for independent tasks.

### Agents (illustrative thin roles)

| Agent | Role | Typical mode | Permissions / notes |
|---|---|---|---|
| Build / Orchestrator | Plans, implements, coordinates, manages completion/continuation decisions | primary | Scoped read/write/shell; delegates specialized work; follows safety gates and approval requirements |
| Architect | Analyzes architecture and proposes solutions | subagent (or all) | **Read-only** (deny implementation edits) |
| Reviewer | Independently reviews implementation and findings | subagent | **Read-only; must not modify implementation files** (deny `edit`; restrict `shell` as needed) |
| Test Runner / Validator | Runs tests, linting, type checks, builds; reports evidence | subagent | Scoped shell/test execution; exact commands/outcomes; never claims unrun checks passed |

In OpenCode, implement as file-based `.opencode/agents/*.md` with `mode`, least-privilege `permissions` (including `subagent` allow/deny), and thin prompts that reference shared skills.

### Portable skills (primary entry procedures, layered)

High-level skills are the **primary entry procedures**; they compose granular skills for reusable/repo-specific sub-procedures.

- **`implementation-workflow`** *(primary, generic)*: understand task/code, identify affected areas/dependencies, define acceptance criteria, plan, implement incrementally, run relevant validation, request independent review, resolve actionable findings, verify final changes. Composes granular skills as needed (e.g. `validate-change`, `prepare-working-branch`, `deliver-pr`, `track-doc-progress`) according to task/context and repo gates.
- **`bug-investigation`** *(primary)*: reproduce, collect evidence, identify root cause, propose smallest appropriate fix, implement and verify (composing `validate-change`/testing as needed).
- **`testing`** *(primary)*: follow existing conventions, run focused checks first, then broader type/lint/build/tests as relevant, verify regressions/edge cases, **never claim a check passed unless executed**. May compose or align with `validate-change` to keep one evidence-based validation procedure.
- **`code-review`** *(primary)*: focus on actual defects (correctness, security, compatibility, missing tests/error handling), classify by severity, give actionable recommendations.

**Granular/repo-specific skills (composed, not competing entry points):** e.g. `pick-next-work-item`, `ensure-green-baseline`, `prepare-working-branch`, `track-doc-progress`, `validate-change`, `deliver-pr`, `babysit-pr`. Keep them focused and invoked/composed by primary workflows or explicit repo-specific entry points without duplicating the same procedure.

### Commands (thin entry points)

| Command | Purpose | Notes |
|---|---|---|
| `/implement <task>` | **Generic** implementation workflow | Thin wrapper: loads/composes `implementation-workflow` (and granular skills as appropriate); not specific to docs/next-feature selection |
| `/investigate <problem>` | Root-cause investigation | Loads/composes `bug-investigation` |
| `/verify` | Run project verification | Loads/composes `testing`/`validate-change`; reports evidence |
| `/loop <task>` | Start autonomous loop | **Deferred** until minimal architecture is validated (optional later) |
| `/cancel-loop` | Cancel active loop | **Deferred** with loop (optional later) |

Commands remain thin wrappers over shared skills/agents and must not embed duplicated full procedures. Repo-specific flows (e.g. “implement next feature from `docs/`”) should be expressed as explicit compositions of primary + granular skills/entry points, preserving required gates.

## 6. This Repo: Concrete Migration and Decomposition

### Current state (to migrate)

- `.cursor/rules/cos-conventions.mdc` (`alwaysApply`): repo-wide conventions, testing, docs, build/tooling, and a review checklist — **shared**; merge into `AGENTS.md` (dedupe/align with existing DoD) as part of standardization.
- `.cursor/rules/frontend-shades.mdc` (scoped to `frontend/**`): **path-scoped shared guidance** → migrate to `frontend/AGENTS.md`.
- `.cursor/rules/sdk-berry-sync.mdc` (`alwaysApply`): cross-cutting invariant → **shared instruction** (`AGENTS.md` and/or relevant nested scope).
- `.cursor/rules/script-examples.mdc` (`alwaysApply`): repeatable procedure → **focused shared skill** (referenced from instructions where needed).
- `.agents/skills/implement-next-feature/SKILL.md`: **monolithic** (preflight/baseline, pick/propose, branch, track docs, implement/validate, on-device artifacts, commit/push/PR, CI iteration) and overlaps `.agents/skills/babysit-pr/SKILL.md` — decompose and re-layer.

### Layering and decomposition (compose rather than duplicate)

- **Primary (generic) entry procedures:** `implementation-workflow`, `bug-investigation`, `testing`, `code-review`.
- **Granular/repo-specific skills composed by them:** `pick-next-work-item`, `ensure-green-baseline`, `prepare-working-branch`, `track-doc-progress`, `validate-change`, `deliver-pr`, `babysit-pr` (and others as justified).
- Preserve required gates and ordering when composed: explicit approval (“go”) before implementation/Git writes where required, green baseline (baseline-fix on a **separate branch/PR** and merged before unrelated feature work), safe branch sync (merge base; no destructive rewrites), evidence-based validation, PRs never merged automatically, and accurate reporting of unrun checks/blockers.

`/implement` remains the **generic** implementation command; the former “implement next feature from docs” behavior becomes an explicit composition (primary workflow + granular skills such as pick-next/baseline/track/deliver/babysit) invoked through an appropriate entry point/workflow, not by making `/implement` docs-specific.

### Migration discipline (all harnesses supported)

- Move/merge shared content first into standard locations; dedupe with `AGENTS.md`.
- **Merge `.cursor/rules` into `AGENTS.md`, nested `AGENTS.md`, and/or `.agents/skills/`** (no parallel rule sources). Keep Cursor-specific adapter files only when a capability cannot be expressed in shared standards.
- Add nested `AGENTS.md` for path-scoped guidance; convert repeatable procedures to skills.
- Introduce thin commands/agents/subagents only when a concrete harness/use-case needs them; support Cursor, OpenCode, and VS Code Copilot with minimal, non-duplicative adapters.
- Validate parity in the harnesses in use before removing `.cursor/rules` content.
- Preserve safety gates and repo-specific invariants (e.g. SDK↔Berry binding parity and compiled device examples).

## 7. Autonomous Loop (Optional, Deferred)

The autonomous loop/orchestrator plugin is **deferred until the minimal non-loop architecture is validated** (shared foundation, layered skills, and thin commands/roles working as intended). Introduce it only as an optional later phase.

If/when adopted (OpenCode-specific, e.g. `.opencode/plugins/agent-loop/` with `/loop`, `/cancel-loop` file-based commands), keep it a small orchestrator responsible for execution/continuation only:

- **Workflow:** store task/state → execute → listen for `session.idle` → check completion/execution state → continue same session or stop (complete/blocked/cancelled/limit).
- **State (illustrative):** `LoopState { sessionID, originalTask, iteration, maxIterations, active }`, persisted (e.g. `ctx.storage`) to handle interruptions/cancellation.
- **Safety defaults:** max iterations 10, identical consecutive failures 2, no-progress iterations 2, max review/fix cycles 3, **automatic commits disabled**, **push/deployment require approval**; prevent concurrent continuation prompts; distinguish completion vs errors/blocking/cancellation/exhaustion; **never treat stopping or step exhaustion as successful completion**.
- **APIs:** `Plugin.define`, `ctx.command.transform()`, `ctx.event.subscribe()`, `ctx.session.prompt()`, `ctx.session.context()`, `ctx.storage`.
- **Completion:** must be evidence-based per §8 (never rely on agent claims alone).

## 8. Completion Criteria (Evidence-Based)

A task is complete only when:

- All acceptance criteria are satisfied.
- Required tests pass.
- Type checking and relevant builds pass.
- No critical or high-priority review findings remain.
- The working tree diff has been inspected.
- The orchestrator explicitly confirms completion.

The completion decision must rely on actual verification results and executed checks—not merely the implementation agent’s claims.

## 9. Implementation Roadmap

### Phase 1: Foundation (shared, standards-based)
- Consolidate shared rules into root `AGENTS.md` (deduped/aligned with existing DoD) by migrating `.cursor/rules/cos-conventions.mdc`, `sdk-berry-sync.mdc`, and relevant shared content.
- Add nested `AGENTS.md` where path-specific guidance is warranted (e.g. `frontend/AGENTS.md` from `frontend-shades.mdc`).
- Establish portable, **layered** skills in `.agents/skills/` (primary entry procedures + focused granular skills) and migrate procedural rules (e.g. `script-examples.mdc`) into skills.
- Remove parallel `.cursor/rules` sources after parity is validated across supported harnesses.

### Phase 2: Thin roles and commands (minimal architecture)
- Define thin roles (orchestrator/architect/reviewer/test-runner) with least-privilege permissions when needed.
- Add thin, **file-based** OpenCode commands/agents/subagents for concrete workflows (`/implement` generic, `/investigate`, `/verify`) following §4.
- Add/keep Cursor and VS Code Copilot adapters only as needed and non-duplicative; ensure all supported harnesses can use the shared foundation.

### Phase 3: Validation of minimal architecture
- Validate discovery/behavior in supported harnesses: `AGENTS.md` + nested instructions, `.agents/skills/`, command/agent/subagent delegation and permissions (reviewer read-only, evidence-based validation).
- Test with small bug fixes and multi-step tasks; confirm no duplicated/competing procedures and that safety gates/invariants hold.

### Phase 4: Autonomous loop (optional, later)
- Only after Phase 3 is validated, implement a small OpenCode loop plugin (`/loop`, `/cancel-loop`) with persistent state, iteration/progress limits, and reliable evidence-based completion detection per §7–§8.

### Phase 5: Ongoing validation
- Test failures, interruptions, cancellation, iteration limits, and repeated failures/no-progress cases (when loop exists).
- Verify the loop never reports false completion and that completion remains evidence-based.
- Re-validate supported harnesses when shared instructions/skills evolve.

## 10. Architectural Guidelines

| Decision                    | Recommendation                                                  |
| --------------------------- | --------------------------------------------------------------- |
| Source of truth             | `AGENTS.md` (+ nested) + `.agents/skills/`                      |
| Rules/instructions format   | Markdown                                                        |
| Skill format                | Agent Skills specification (portable, layered)                  |
| Workflow entry points       | Thin, tool-specific commands composing shared skills            |
| `/implement`                | Generic implementation workflow (not docs-specific)             |
| Agent/subagent definitions  | Thin, native, tool-specific; prefer file-based in OpenCode      |
| Loop/continuation           | Small tool-specific orchestrator/plugin; deferred; evidence-based |
| Shared references           | Skill directories or `docs/`                                    |
| Tool-specific configuration | Minimal; only for real capability/runtime needs                 |
| Harness support             | Cursor, OpenCode, VS Code Copilot; avoid duplication            |
| `.cursor` rules migration   | Merge into shared standard locations; no parallel rule sources  |
| Configuration generation    | Avoid initially                                                 |
| Symlinks / sync scripts     | Avoid                                                           |
| Execution style             | Sequential by default; parallel only for independent tasks      |
| Autonomous side effects     | Limit commits/push/deploy/publish; require approval as needed   |

## 11. Guiding Principles

1. One source of truth for shared instructions and reusable workflows.
2. Prefer open/shared standards over vendor-specific formats.
3. Keep instructions, skills, commands, agents, loop/control, and runtime config separate.
4. **Layer skills:** primary (portable) entry procedures compose granular skills; avoid monoliths and duplication.
5. Avoid unnecessary abstraction, multi-agent complexity, symlinks, installation steps, and generators.
6. Use native features when interoperability is not practical; keep adapters thin and non-duplicative across supported harnesses.
7. Make verification evidence-based; never treat stopping/step exhaustion as completion.
8. Optimize context usage, especially when running local LLMs.
9. Design for future agent harnesses without depending on them.

## 12. Validation Checklist

When validating the shared/adapted setup in each supported harness:

- Does it discover root `AGENTS.md` and respect nested instructions?
- Does it discover `.agents/skills/` and load references/supporting files?
- Do commands trigger the intended thin entry points (with `/implement` as the generic workflow)?
- Can primary agents delegate to intended subagents with correct least-privilege permissions (e.g. reviewer/architect read-only, scoped test-runner, explicit `subagent` allow/deny)?
- Are layered skills composed without duplicated/competing procedures or lost gates?
- Has `.cursor/rules` content been migrated to shared standard locations with no parallel rule sources (Cursor-specific kept only if unavoidable)?
- If/when a loop exists (deferred): does it persist state, handle cancellation/failures/limits, avoid false completion, and enforce no auto-commit/push?

## Final Architecture

**Shared foundation (portable):**

- `AGENTS.md` (+ nested `AGENTS.md` where needed)
- `.agents/skills/` (layered: primary entry procedures composing granular skills)

**Tool-specific adapters (thin, created only as needed, all supported harnesses):**

- `.cursor/` (minimal; shared rules migrated to standards)
- `.opencode/` (prefer file-based agents/commands per OpenCode rules; loop plugin deferred)
- `.github/`

**Recommended starting point:** Implement the shared foundation and layered portable skills first, add thin file-based roles/commands with least-privilege permissions when needed, validate the minimal architecture across supported harnesses, and only then consider a lightweight loop plugin. The objective is not to force every tool into an identical configuration format, but to maintain shared knowledge and workflows while allowing each harness to use its native execution capabilities without duplication.
