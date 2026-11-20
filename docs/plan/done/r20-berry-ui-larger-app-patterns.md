# R20d: Berry/UI larger app patterns + reference template

- **ID:** r20-berry-ui-larger-app-patterns
- **Status:** done (delivered, PR/CI evidence recorded; pending human review/approval, merge, and required on-device CYD smoke validation)
- **Created:** 2026-11-18
- **Source/links:** [`../needs-clarification/r20-deferred-backlog.md`](../needs-clarification/r20-deferred-backlog.md) (umbrella, item d: larger apps); [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (R20); guide [`../../berry-ui-larger-app-patterns.md`](../../berry-ui-larger-app-patterns.md); template [`../../../storage/ts/apps/onboarding.ts`](../../../storage/ts/apps/onboarding.ts), compiled [`../../../storage/flash_data/apps/onboarding.be`](../../../storage/flash_data/apps/onboarding.be); existing UI examples (e.g. Tap Dash/Breakout in `storage/flash_data/apps` and sources)
- **Approval:** user (2026-11-20, clarify-feature conversation) — explicit approval to mark ready and move to `docs/plan/ready/`. Pre-approved for implementation/PR flow per global gates; merge remains human.

## Summary
Delivered minimal, documented patterns for building larger/stateful Berry UI apps on ESP32 (multi-screen/state navigation, lifecycle notes, memory/stack pitfalls) and one vetted reference template app using only current UI/net APIs. Goal: move beyond demo-only UI feel toward practical app structure guidance.

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
- [x] Pattern guide/spec covering: app structure (single-class/screen-method pattern), navigation/state model (in-memory screen switching via `ui.clear(0)` + rebuild), lifecycle/teardown expectations (`setup`, timer/callback cleanup considerations, `ui.exit()`), and ESP32 memory/stack pitfalls. Delivered in [`docs/berry-ui-larger-app-patterns.md`](../../berry-ui-larger-app-patterns.md).
- [x] Reference onboarding template implemented (3 screens: Greeting → Wi‑Fi status/guidance read-only via `net.wifi()`/`net.host()` with graceful fallback/guidance text, no scanning/password UI → Tips + Done/Exit), demo-only, in-memory navigation. Source: `storage/ts/apps/onboarding.ts`, compiled: `storage/flash_data/apps/onboarding.be` (repo compile-storage workflow; expected script list updated in `frontend/tools/compile-storage.test.ts`).
- [x] Validation steps documented: automated commands per scope and an on-device smoke checklist (screen navigation, status/guidance display behavior when Wi‑Fi state varies, exit/restart behavior) in the guide and this item. Heap/watermark notes: **not captured** (hardware unavailable in this session; capture optional/lightweight when CYD validation is performed).
- [x] UI/API gaps discovered listed as follow-ups in the guide (no speculative API implementation): no text input/provisioning UI, no scan/list API for UI selection in scope, no built-in screen stack/navigation container, limited layout helpers, no standardized timer/cancellation helper beyond `ui.timer` pattern. No additional in-scope gaps requiring implementation in this item.

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
- 2026-11-20 (implementation, branch `feat/r20d-berry-ui-larger-app-patterns` from `origin/main@54c61c0`): added `storage/ts/apps/onboarding.ts` (3-screen demo-only onboarding template, in-memory navigation, read-only `net.wifi()`/`net.host()` status with graceful fallback/guidance, Tips + Done/`ui.exit()`), compiled `storage/flash_data/apps/onboarding.be` via `frontend/tools/compile-storage.ts --write`, updated `frontend/tools/compile-storage.test.ts` expected app list, and added guide `docs/berry-ui-larger-app-patterns.md`. Moved this item `ready/` → `done/` and updated plan/backlog cross-links for R20d scope.
- **Automated validation (exact, this session):**
  - `git status --short --branch` (baseline on new branch): clean tracking `origin/main`.
  - `git fetch origin main --quiet` (exit 0).
  - `cd frontend; npx --no-install tsx tools/compile-storage.ts --write` → `Wrote 3 Berry files to storage\flash_data` (after fixing unsupported ternary expressions in onboarding TS; unrelated `storage/flash_data/scripts/dir.be`,`ls.be` EOL-only rewrites restored to base).
  - `make format-check` → **pass** (all files formatted correctly).
  - `make test` → **pass** (CTest: 31/31 tests passed, 0 failed; Python `scripts` unittest: `Ran 3 tests ... OK`).
  - `cd frontend; yarn format:check` → **pass** (`Prettier ... All matched files use Prettier code style!`).
  - `cd frontend; yarn lint` → **pass** (exit 0).
  - `cd frontend; yarn test` → **pass** (vitest: 18 test files passed, 160 tests passed).
  - `cd frontend; yarn build` → **pass** (`tsc && vite build` completed; Vite built successfully with chunk-size warnings only).
  - `cd frontend; yarn check-storage` (compile-storage verification / `vitest run tools/compile-storage.test.ts`) → **pass** (1 file, 7 tests passed).
- **Not run (blockers):** `make build`, `make lint`, integration/QEMU targets not run — ESP-IDF v6.1 / clang-tidy / QEMU environment not verified in this session (CI is relied on for these; rerun in sourced ESP-IDF v6.1 env per root `AGENTS.md` / README if needed locally). **On-device CYD smoke not run** (no hardware/serial in session).
- **On-device smoke checklist (required, human):** load/run `apps/onboarding.be` on CYD → verify Greeting shows welcome + Next; Wi‑Fi status/guidance screen shows read-only status or graceful fallback/guidance when Wi‑Fi state varies (no scan/password UI); Tips screen lists guidance; Back/Next navigation works across all 3 screens; Done exits cleanly (`ui.exit()` behavior); app restarts cleanly (demo-only). Optionally capture heap/watermarks before/after key transitions if easy. **Merge-blocking until validated with evidence or explicitly waived by a human** (see PR comment).
- **Remaining human actions:** PR review/approval, required on-device CYD smoke validation + evidence (or explicit waiver), and merge (squash expected). No hardware heap/watermark measurements recorded in this session.
