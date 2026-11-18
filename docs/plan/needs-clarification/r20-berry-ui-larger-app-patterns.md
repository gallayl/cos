# R20d: Berry/UI larger app patterns + reference template

- **ID:** r20-berry-ui-larger-app-patterns
- **Status:** needs-clarification
- **Created:** 2026-11-18
- **Source/links:** docs/plan/needs-clarification/r20-deferred-backlog.md (umbrella, item d: larger apps); docs/plan/consolidated-improvement-plan.md (R20); existing UI examples (e.g. Tap Dash/Breakout in storage/flash_data/apps and sources)
- **Approval:** pending (ready/pre-approved asked after feature specification)

## Summary
Define minimal, documented patterns for building larger/stateful Berry UI apps on ESP32 (multi-screen/state navigation, lifecycle notes, memory/stack pitfalls) and deliver one vetted reference template app. Goal: move beyond demo-only UI feel toward practical app structure guidance.

## Scope
- Document conventions: app structure, state management approach, screen/navigation patterns, input handling patterns, and teardown/cleanup expectations consistent with Berry UI runtime.
- Implement or specify one reference “larger app” template (e.g. settings + log viewer or simple dashboard) with constrained resource usage and clear comments.
- Define acceptance criteria and validation plan (build/compile checks, on-device smoke when feasible).

## Non-goals
- Full UI framework or widget library redesign.
- Automatic code generation tooling beyond a template/example.
- Broad feature expansion unrelated to the reference template and documented patterns.

## Acceptance criteria
- [ ] Pattern doc/spec approved (sections listed) covering structure, navigation/state, lifecycle, and ESP32 constraints.
- [ ] One reference template app specified with source location and expected behavior; implemented only after approval/Ready.
- [ ] Validation steps defined: frontend/compile checks and on-device smoke checklist (memory/behavior notes).

## Validation plan
- Automated: make format-check, make test as applicable; frontend checks (yarn format:check/lint/test/build) if TS sources/templates added under frontend scope; compile-storage verification if storage examples updated (per AGENTS.md).
- Hardware/manual: required for on-device UX/memory sanity of the reference template; record heap/watermark observations if feasible.

## Expected areas/files
- storage/ts/apps/ and/or storage/flash_data/apps/ (reference template), frontend/src/cos-sdk/ui.d.ts and docs if APIs clarified, docs/ (patterns guide) or docs/reports as appropriate.

## Dependencies / prerequisites
- R20 umbrella (docs/plan/needs-clarification/r20-deferred-backlog.md).
- Existing Berry UI runtime and examples as baseline.

## Risks / constraints
- ESP32 RAM/stack limits; larger apps can easily fragment or OOM without discipline.
- Patterns must stay aligned with current UI API (avoid speculative APIs unless approved separately).

## Open questions
- [ ] Which reference app is best first template (settings + log viewer, simple dashboard, notes list, etc.)?
- [ ] Should patterns live in docs/ (guides) or as in-repo annotated example(s) only?
- [ ] Any required UI API gaps to call out (separately tracked) vs working within current API?

## Progress / evidence
- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 4).
