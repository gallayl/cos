# Cross-IDE / Agent Harness Configuration Strategy

## Goal

Maintain one Git-native source of truth for AI coding instructions, rules, skills, and workflows that works across Cursor, OpenCode, VS Code Copilot, and future agent harnesses.

**Core principle:** Standardize shared content, use native tool-specific configurations only when necessary.

## 1. Adopt These Standards

| Component            | Standard / Location         | Purpose                                 |
| -------------------- | --------------------------- | --------------------------------------- |
| Project instructions | `AGENTS.md`                 | Shared repository rules and conventions |
| Nested instructions  | `**/AGENTS.md`              | Directory-specific context              |
| Skills               | `.agents/skills/`           | Reusable workflows and domain knowledge |
| Custom agents        | Tool-specific directories   | Agent roles, permissions and tools      |
| Commands             | Tool-specific directories   | Explicitly executable prompts           |
| Hooks                | Tool-specific configuration | Lifecycle automation                    |
| MCP                  | Tool-specific configuration | External tool integrations              |

### Shared foundation

**`AGENTS.md`**

- Architecture and repository structure.
- Coding conventions and standards.
- Technology choices and constraints.
- Build, test, lint and formatting commands.
- Definition of done.
- References to relevant skills.

**Agent Skills (`.agents/skills/`)**

- Use the [Agent Skills specification](https://agentskills.io/).
- Each skill has a `SKILL.md` with YAML frontmatter.
- Keep skills focused on individual workflows.
- Store supporting documentation, scripts and references alongside the skill.
- Avoid duplicating general project rules inside skills.

## 2. Recommended Repository Structure

```text
repository/
│
├── AGENTS.md
│
├── packages/
│   ├── core/
│   │   └── AGENTS.md
│   └── frontend/
│       └── AGENTS.md
│
├── .agents/
│   └── skills/
│       ├── implement-feature/
│       │   ├── SKILL.md
│       │   └── references/
│       ├── code-review/
│       │   ├── SKILL.md
│       │   └── references/
│       ├── write-tests/
│       │   └── SKILL.md
│       └── release/
│           └── SKILL.md
│
├── .cursor/
│   ├── agents/
│   │   ├── architect.md
│   │   └── reviewer.md
│   └── rules/              # Cursor-specific only
│
├── .opencode/
│   └── agents/
│       ├── architect.md
│       └── reviewer.md
│
└── .github/
    ├── agents/
    │   ├── architect.agent.md
    │   └── reviewer.agent.md
    └── instructions/       # Copilot-specific only
```

Do not create directories until there is an actual need for them.

## 3. Separation of Responsibilities

Keep the concepts clearly separated:

- **Rules:** What agents must consistently know or obey.
- **Skills:** How to perform specific tasks.
- **Agents:** Specialized workers with particular responsibilities, tools and permissions.
- **Commands:** Explicitly executable prompts or workflows.
- **Hooks:** Automation triggered by lifecycle events.
- **MCP:** External tools and integrations.

## 4. Custom Agents: Use Native Adapters

Agent definitions are not fully standardized because different harnesses expose different capabilities.

Use native definitions:

| Tool            | Location            |
| --------------- | ------------------- |
| Cursor          | `.cursor/agents/`   |
| OpenCode        | `.opencode/agents/` |
| VS Code Copilot | `.github/agents/`   |

Avoid duplicating complete agent instructions.

Instead:

1. Define reusable workflows in `.agents/skills/`.
2. Reference or invoke shared skills from native agent definitions.
3. Keep tool-specific configuration limited to capabilities, permissions, models and execution settings.

## 5. Architectural Guidelines

| Decision                    | Recommendation                  |
| --------------------------- | ------------------------------- |
| Source of truth             | `AGENTS.md` + `.agents/skills/` |
| Rules format                | Markdown                        |
| Skill format                | Agent Skills specification      |
| Agent definitions           | Native tool-specific files      |
| Shared references           | Skill directories or `docs/`    |
| Tool-specific configuration | Minimal                         |
| Configuration generation    | Avoid initially                 |
| Symlinks                    | Avoid                           |
| Installation scripts        | Avoid                           |
| Manual synchronization      | Avoid                           |

## 6. Implementation Plan

### Phase 1: Establish the foundation

- Create root `AGENTS.md`.
- Define repository architecture and coding conventions.
- Add nested `AGENTS.md` files only where necessary.
- Create `.agents/skills/`.

### Phase 2: Extract reusable skills

Start with:

- `implement-feature`
- `code-review`
- `write-tests`
- `release`

Move task-specific workflows and supporting documentation into these skills.

### Phase 3: Add native agents

- Create Cursor agent definitions.
- Create OpenCode agent definitions.
- Create Copilot agent definitions.
- Reuse shared skills instead of duplicating prompts.

### Phase 4: Validate compatibility

Test each tool independently:

- Does it discover root `AGENTS.md`?
- Does it respect nested instructions?
- Does it discover `.agents/skills/`?
- Can it load skill references and supporting files?
- Can its native agents access and invoke the shared workflows?

## 7. Guiding Principles

1. One source of truth for shared instructions.
2. Prefer open standards over vendor-specific formats.
3. Keep instructions, skills and agents separate.
4. Avoid unnecessary abstraction.
5. Avoid symlinks, installation steps and configuration generators.
6. Use native features when interoperability is not practical.
7. Design for future agent harnesses without depending on them.

## Final Architecture

**Shared foundation:**

- `AGENTS.md`
- `.agents/skills/`

**Tool-specific adapters:**

- `.cursor/`
- `.opencode/`
- `.github/`

The objective is not to force every tool into an identical configuration format. It is to maintain shared knowledge and workflows while allowing each harness to use its native execution capabilities.

**Recommended starting point:** Implement `AGENTS.md` and `.agents/skills/` first. Add tool-specific agent definitions only when actual requirements emerge.
