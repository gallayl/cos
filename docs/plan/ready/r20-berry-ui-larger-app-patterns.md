# R20d: Berry/UI larger app patterns + reference template

- **ID:** r20-berry-ui-larger-app-patterns
- **Status:** ready (approved)
- **Created:** 2026-11-18
- **Source/links:** [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (umbrella, item d: larger apps); [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20); existing UI examples (e.g. Tap Dash/Breakout in `storage/flash_data/apps` and sources)
- **Approval:** user (2026-11-20, clarify-feature conversation) — explicit approval to mark ready and move to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.

## Summary
Define minimal, documented patterns for building larger/stateful Berry UI apps on ESP32 (multi-screen/state navigation, lifecycle notes, memory/stack pitfalls) and deliver one vetted reference template app. Goal: move beyond demo-only UI feel toward practical app structure guidance, using only current UI/net APIs.

## Scope (decisions recorded 2026-11-18)
- **Reference template (1D-A-feasible, demo-only):** an **onboarding-style** app with **3 screens**, launched as a demo each time (no “onboarding completed” persistence):
  1. **Greeting** — welcome text + Next.
  2. **Wi‑Fi status/guidance (read-only, current API)** — display status derived from available `net` APIs when feasible (e.g. `net.wifi()` / `net.host()`), plus short guidance text (e.g. how to configure via shell/scripts such as `wifi`/`net.wifi_connect(...)`). **No** Wi‑Fi network scanning UI, **no** password entry UI (not supported by current widgets/APIs). If exotic/unsupported connection cases are mentioned, keep to guidance text only (no device-side special handling beyond what APIs allow).
  3. **Tips + Done/Exit** — short list of tips + Done/Exit (uses `ui.exit()` or equivalent pattern consistent with examples).
- **Patterns + artifacts (2A):** provide **in-repo annotated example(s)** (e.g. TS app under `storage/ts/apps/` with compiled artifact handling per repo workflow) **plus a short guide doc** in `docs/` (or a clearly linked doc page) covering structure, navigation/state, lifecycle/teardown expectations, and ESP32 constraints/pitfalls.
- **API gaps (3A):** **work within current UI API**; document patterns only (e.g. screen switching via rebuilding/replacing UI content or managing widget visibility/layout patterns consistent with current capabilities). Call out any observed gaps as **follow-ups to track separately** (do not implement speculative APIs in this item).
- **Validation emphasis (4A):** `make format-check`, `make test` as applicable; frontend checks (`yarn format:check`/`lint`/`test`/`build`) if TS/template sources added under frontend scope; `compile-storage` verification if storage examples updated (per `AGENTS.md`). **On-device smoke checklist required** for the reference template (navigation works, no obvious crashes); record heap/watermark observations if feasible (lightweight). Demo-only means no persistence validation required.

## Non-goals
- Full UI framework/widget library redesign or new widgets (especially text input) in this item.
- Wi‑Fi scanning UI, interactive password entry, or full Wi‑Fi provisioning flows in the template (only read-only status/guidance as above).
- Persistence of onboarding completion state.
- Automatic code generation tooling beyond the template/example.
- Broad feature expansion unrelated to the reference template and documented patterns.

## Acceptance criteria
- [ ] Pattern guide/spec approved covering: app structure (single-class/screen modules pattern as chosen), navigation/state model (in-memory screen switching), lifecycle/teardown expectations (`setup`, timers/callback cleanup considerations), and ESP32 memory/stack pitfalls.
- [ ] Reference onboarding template specified (3 screens as above, demo-only) with expected file locations (`storage/ts/apps/...` and compiled outputs per repo rules) and behavior; implemented only after approval/Ready transition.
- [ ] Validation steps documented: automated commands per scope and an on-device smoke checklist (screen navigation, status/guidance display behavior when Wi‑Fi state varies, exit behavior). Any heap/watermark notes recorded if captured.
- [ ] Any UI/API gaps discovered are listed as follow-ups (separately tracked) with no speculative API implementation in this item.

## Validation plan
- **Automated:** `make format-check`, `make test` as applicable. If frontend/TS/template sources touched: from `frontend/` run `yarn format:check`, `yarn lint`, `yarn test`, `yarn build`. If `storage/ts`/compiled `storage/flash_data` examples updated, run `compile-storage` verification/checks per `AGENTS.md`/repo tooling.
- **Hardware/manual (required for template):** on CYD, load/run the onboarding template: verify Greeting → Wi‑Fi status/guidance → Tips/Done flow, navigation works, exit works, app restarts cleanly (demo-only). Optionally note heap/watermarks before/after key transitions if easy; record blockers if hardware unavailable.

## Expected areas/files (rough)
- `storage/ts/apps/` (onboarding template source), `storage/flash_data/apps/` (compiled output as per workflow)
- `docs/` guide page for Berry/UI larger-app patterns (name/location to be finalized in implementation planning)
- `frontend/src/cos-sdk/ui.d.ts` / `net` docs references only if documentation clarifications needed (no API changes required by this item)

## Dependencies / prerequisites
- R20 umbrella: [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (item d).
- Existing Berry UI runtime/examples (`tap_dash.ts`, `breakout.ts`) and `net.wifi()`/`net.host()` availability for status display (read-only guidance if calls unavailable/fail).
- Current UI widget set (Label/Button/Checkbox/GroupBox) sufficient for the 3-screen flow.

## Risks / constraints
- ESP32 RAM/stack and fragmentation risk; template and guide must emphasize small, explicit UI trees and cleanup habits.
- Wi‑Fi status reads may fail or be unavailable depending on state; template must degrade gracefully to guidance text.
- No text input/scanning means the “real setup” remains external (shell/scripts) — document this clearly to avoid user confusion.

## Open questions
- None (decisions recorded 2026-11-18: 1D-A-feasible onboarding 3 screens, demo-only; 2A in-repo annotated example + short guide doc; 3A work within current API, gaps as follow-ups; 4A automated + required on-device smoke with lightweight memory notes if feasible). If implementation reveals a hard blocker (e.g. `net` calls not usable in UI context as assumed), document it and do not claim Ready without resolution.

## Progress / evidence
- 2026-11-18: created from analysis of done/ready features and R20 dangling backlog follow-ups (proposed as new feature idea 4).
- 2026-11-18 (clarify-feature): reviewed existing UI examples and `network.d.ts`. User choices recorded (onboarding 3-screen demo-only template, guide + annotated example, current API only, automated + on-device smoke). Open questions resolved.
- 2026-11-20 (clarify-feature, user approval): explicit approval recorded (user, clarify-feature conversation) to mark **ready (approved)** and move from `docs/plan/needs-clarification/` to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.
