# Berry/UI larger app patterns (R20d)

Short guide for building larger/stateful Berry UI apps on ESP32 using the **current** `ui`/`net` APIs. See the vetted reference template: [`storage/ts/apps/onboarding.ts`](../storage/ts/apps/onboarding.ts) (compiled: `storage/flash_data/apps/onboarding.be`).

## Scope and goals

- Practical structure for demo-to-app growth without a UI framework redesign.
- Patterns only; work within the current widget/runtime API. Document gaps as follow-ups (no speculative API work in R20d).

## App structure (template pattern)

- **One UI app class per screen flow** implementing `CosUiApplication` with `setup(content, width, height)`.
- Keep screen state explicit as fields (e.g. current screen id, dimensions). Prefer small, plain values.
- Split rendering into screen-specific methods (`renderGreeting`, `renderWifiStatus`, `renderTips`) called from a central `render()`/navigation method.
- Rebuild the visible UI on navigation: `ui.clear(0)` then create only the widgets needed for the active screen. This matches current capabilities (no virtual screen stack/widget show/hide API required).

## Navigation and state

- **In-memory navigation only** (demo): store the current screen index/id in a field and call `render()` after changes.
- Pass navigation via button callbacks (`ui.on_click`) that update state then rebuild. Avoid deep callback chains holding large captured objects.
- Do not persist onboarding completion/state in this template; persistence is out of scope.

## Lifecycle and teardown

- `setup` receives display dimensions; store them for layout calculations.
- **Timers/callbacks:** if you start `ui.timer(...)`, ensure callbacks handle app-exit state safely. The template uses no timers; when used, prefer patterns that stop work when the app is no longer active and avoid leaking work across restarts.
- **Exit:** call `ui.exit()` to close the app (and `ui.clear(0)` if you want an explicit visual reset before exit, as in the template). `teardown()` can be added when cleanup is needed; keep it minimal and deterministic.
- **Restart behavior:** demo apps should be restartable/clean. Recreating widgets on each screen keeps ownership simple.

## Wi-Fi status/guidance (read-only, current API)

- Use `net.wifi()` and/or `net.host()` when available to display status (connected, mode, SSID, IP as available).
- **Graceful fallback:** calls may fail, return `ok: false`, or be unavailable depending on state/context. Show guidance text instead of crashing (wrap reads if needed, check `ok`/fields).
- **Guidance only:** tell users to configure via shell/scripts (e.g. `wifi`, `net.wifi_connect(ssid, password)`). No Wi-Fi scanning UI and no password entry UI (not supported by current widgets/APIs). Exotic connection cases: guidance text only.

## ESP32 memory/stack pitfalls

- **Small UI trees:** create only active-screen widgets; clear before rebuilding to avoid accumulating unused widgets/state.
- **Bounded strings/locals:** avoid large temporary strings or deep structures in hot paths/callbacks; prefer short labels and simple formatting.
- **Avoid fragmentation-prone patterns:** repeated allocate/free in timers or tight loops can fragment heap; keep per-frame/per-tick work minimal.
- **Stack:** keep call depth and local object sizes modest in Berry/script callbacks.
- **No hidden persistence:** do not assume NVS/storage writes for demo flows unless explicitly designed and validated.

## API gaps / follow-ups (tracked separately)

Observed/current-scope gaps to address in future items (do not implement speculatively here):

- No text input widget; interactive credential entry/provisioning flows not possible in pure UI.
- No Wi-Fi scan/list API surfaced for UI selection in this template scope.
- No built-in screen stack/navigation container; screen switching is manual rebuild/clear.
- Limited layout helpers; positioning is manual pixel math.
- No standardized timer/cancellation/lifecycle helper shown in SDK beyond `ui.timer` callback pattern—apps must be careful with cleanup.

## Validation expectations (R20d)

- **Automated (per scope):** `make format-check`, `make test` as applicable; from `frontend/` `yarn format:check`, `yarn lint`, `yarn test`, `yarn build` when TS/template sources change; compile-storage verification when `storage/ts`/`storage/flash_data` examples change.
- **Hardware/manual (required for template):** on CYD load/run `apps/onboarding.be`: Greeting → Wi-Fi status/guidance → Tips/Done flow, navigation works, Done/exit works, app restarts cleanly (demo-only). Optionally note heap/watermarks before/after key transitions if easy. Record blockers if hardware unavailable.

## Reference template

- Source: `storage/ts/apps/onboarding.ts`
- Compiled: `storage/flash_data/apps/onboarding.be`
- Screens: Greeting (welcome + Next), Wi-Fi status/guidance (read-only via `net.wifi()`/`net.host()` + shell guidance), Tips + Done/Exit (`ui.exit()`).
