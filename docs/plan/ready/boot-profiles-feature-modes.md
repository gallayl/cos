# Boot profiles and service mode selection

- **ID:** boot-profiles-feature-modes
- **Status:** ready (approved)
- **Created:** 2026-11-16
- **Updated:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `main/main.c` fixed boot/init wiring, `components/text_console/src/text_console.cpp` console task/stdout hook, and audit recommendation to trade features for memory. Tracker: [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (2026-11-16 audit follow-ups).
- **Approval:** User approved the drafted specification in clarify-feature conversation on 2026-11-16 ("Approve spec as drafted"). Pre-approved for implementation/PR flow (merge remains human).
- **Open questions:** none

## Summary
Boot currently initializes a fixed set of services/features. v1 defines two boot profiles selected by a persisted NVS setting (applied on next boot) to allow an explicit memory/feature tradeoff while preserving local recovery.

## Scope (v1)
- Define two profiles: `default` and `low-memory-offline`.
- Persist selected profile in NVS; provide a shell command to get/set/clear it. Changes apply on **next boot/restart** only.
- Gate boot-time init/start/wiring per profile (v1 may be implemented in `main.c`/boot flow before full `unified-service-lifecycle` exists; document limitations).
- Keep a local recovery/debug path in every v1 profile.
- Document per-profile service availability and record measured memory evidence (qualitative acceptance; no guessed hard KB targets).

## Non-goals (v1)
- Runtime profile switching, hot stop/start of services after boot, or general runtime reconfiguration.
- Non-interactive ultra-minimal profiles (deferred).
- Automatic boot-failure rollback/self-healing beyond fallback on missing/invalid/corrupt stored value.
- UI redesign beyond necessary status/output for the profile command.
- Changing safety-critical hardware bring-up.
- Full `unified-service-lifecycle` framework unless required to satisfy this spec (v1 allows gated init).

## Profile definitions (proposed for approval)

### `default`
- **Intent:** preserve current behavior as far as dependencies allow.
- **Services/init:** same boot sequence and wiring as current `main/app_main` (display, calibration, light sensor, RGB LED, brightness, text console, I2C bus, filesystem, Wi-Fi, network, Bluetooth, time sync, HTTP server, WebSocket, system, shell; dependent wiring such as keyboard routing, console output broadcast, WebSocket text input as today).

### `low-memory-offline`
- **Intent:** reduce memory/task/network overhead for offline/local use while keeping local recovery.
- **Enabled/kept (recommended matrix):** display, calibration, light sensor, RGB LED, brightness, text console, I2C bus, filesystem, system, shell, Berry runtime/autoexec (and local recovery commands/scripts including profile selection). Safety-critical/core bring-up remains unchanged.
- **Disabled/skipped:** Wi-Fi (`wifi_init`), network diagnostics/HTTP client worker (`network_init`), Bluetooth (`bluetooth_init`), time sync (`time_sync_init`), HTTP server (`http_server_init`), WebSocket (`websocket_init`) and dependent wiring (network/BT routes, console output broadcast to WebSocket, WebSocket text input). No auto-connect/network/BT/server behavior.
- **Unavailable behavior (expected):** Wi-Fi/BT/HTTP/network/time-sync commands/features fail or are unavailable as appropriate for uninitialized services; remote console/input over WebSocket unavailable. Serial/on-device local recovery remains.

## Selection, persistence, and command contract (proposed for approval)
- **Mechanism:** NVS-persisted profile value read at boot and applied during startup.
- **Precedence (resolved, 2026-11-16 review):** **Boot profile is highest priority at boot.** It selects the allowed service set and wiring for that boot. Effective behavior: **`effective = profile allows service ? persisted/service setting (if defined by that service item) : forced disabled/off`** regardless of any persisted toggle. In `low-memory-offline`, Wi‑Fi/network/BT/HTTP/WS/time-sync remain disabled even if a persisted "enabled" toggle exists; in `default`, persisted toggles may be honored only where the service plan explicitly defines them. `boot_profile`/service status output must make forced-off-by-profile vs user-disabled state distinguishable when feasible; document effective behavior in this plan and in each affected service plan.
- **Command:** `boot_profile` (shell command)
  - `boot_profile` — print stored value (or `default`/none), effective profile for next boot/current boot as applicable, and supported names.
  - `boot_profile set <profile>` — persist `default` or `low-memory-offline`; report that it takes effect on next boot/restart.
  - `boot_profile clear` — remove stored selection; equivalent to `default` on next boot.
  - Invalid profile name: return/report error, do not change stored value.
- **NVS details:** namespace/key and stored string values to be finalized in implementation/docs (must unambiguously represent `default` and `low-memory-offline`).
- **Fallback:** missing, invalid, or corrupt stored value => treat as `default` on boot (safe fallback). No automatic boot-failure rollback.

## Recovery/debug contract (v1)
- **Required in every v1 profile:** UART/USB serial shell input/output and on-device text console remain functional; filesystem access and ability to run recovery commands/scripts (including `boot_profile` changes for next boot) remain available. Keyboard/input routing remains as available for initialized components. Console lifecycle/visibility semantics (including hidden/app-owned: UART/stdout forwarding and output listener callbacks stay active, console buffer/history preserved while render/touch suspended) must remain consistent with [`./text-console-memory-lifecycle.md`](./text-console-memory-lifecycle.md) (source of truth).
- **Not in v1:** profiles that omit shell/console/UI or rely on rebuild/reflash/external-only recovery.

## Acceptance criteria / definition of done
- [ ] `default` profile preserves current startup behavior and service availability (subject to existing hardware/environment conditions).
- [ ] `low-memory-offline` applies the agreed matrix: disabled services are not initialized/wired; enabled recovery/core/local-runtime behavior works.
- [ ] `boot_profile` supports get/set/clear with correct persistence, user-visible output/errors, and "takes effect on next boot" semantics.
- [ ] Missing/invalid/corrupt persisted profile value falls back to `default` at boot.
- [ ] Local recovery path (serial shell + on-device console + filesystem + profile command) works in both profiles.
- [ ] Documentation (and/or spec notes) describes per-profile feature availability and memory/feature tradeoffs; measured memory evidence is recorded for comparison (no guessed hard KB targets).
- [ ] Automated checks required by `AGENTS.md` for affected scope pass; required on-device/manual validation completed or explicitly reported with blockers.

## Validation plan
- **Automated (affected scope):** `make format-check`, `make test`; run `make build` and `make lint` when ESP-IDF/clang-tidy environment is available (firmware/build/lint-covered changes). Add/update host/unit tests where feasible (e.g. profile parsing/selection/fallback logic if extracted/testable). Follow `AGENTS.md` reporting rules.
- **Hardware/manual (required):** on device, for each profile: set/clear via `boot_profile`, reboot, verify effective profile and service availability/unavailability (including disabled network/BT/server behavior in `low-memory-offline`), verify serial shell + on-device console recovery path, and capture memory evidence (e.g. `mem`, `berry profile`/heap/watermarks/task-related indicators as available) compared to `default` baseline in comparable conditions. Record results/evidence.

## Expected areas/files (rough)
- `main/main.c` (boot-time profile gating/wiring)
- New or existing config/profile helper (NVS read/write/validate/fallback), likely under a suitable component or `main/`
- `components/shell/` or shell command registration for `boot_profile`
- `components/system/` and/or docs/help text if command discoverability/output is surfaced there
- `docs/plan/consolidated-improvement-plan.md` (cross-link/status consistency when applicable)
- Possibly component init guards/docs depending on implementation details

## Dependencies / prerequisites
- Related to [`./unified-service-lifecycle.md`](./unified-service-lifecycle.md) (ready, shared lifecycle contract/base). **v1 may proceed via boot-time gated init/wiring before full unified lifecycle lands**, with limitations documented, but implementation should **align with and migrate to** unified descriptors/interfaces when available (avoid duplicate boot-wiring logic).
- Interacts with service-specific lifecycle behavior: HTTP/WebSocket (`./optional-http-websocket-service.md`), network diagnostics, Wi-Fi (`./wifi-managed-start-stop.md`), console (`./text-console-memory-lifecycle.md`, source of truth for console lifecycle semantics), shell, Bluetooth/time sync (as applicable to disabled/kept services).
- `service-memory-budget-observability` (`./service-memory-budget-observability.md`) is useful for evidence-based comparison but v1 acceptance relies on qualitative availability plus measured evidence (no hard targets).

## Implementation order / prerequisites (resolved, 2026-11-16 review)
- **Lifecycle first:** `unified-service-lifecycle` descriptors/states/capabilities/boot helpers must land before new permanent profile/service-gating logic is added. Do not introduce or keep a second permanent ad-hoc boot/service model.
- **Temporary migration only:** gated boot init in `main.c` is allowed only as an explicitly time-limited migration step, documented as temporary and planned to migrate to descriptor-driven gating. New service-gating work must use unified-lifecycle interfaces/hooks once available.
- Coordinate with `optional-http-websocket-service` and `wifi-managed-start-stop` so disabled/stopped services and recovery guarantees are represented consistently via unified states/capabilities.
- **Semantic prerequisite:** `text-console-memory-lifecycle.md` is the source of truth for console lifecycle/visibility semantics and recovery guarantees referenced by this plan; keep profile behavior/docs consistent with it. Reuse `service-memory-budget-observability` evidence/checklists for per-profile memory measurements where useful.

## Risks / constraints
- ESP32 RAM/flash/task constraints; disabling services must not break required recovery/core bring-up.
- Service dependencies must be respected (e.g. WebSocket depends on HTTP server; network features depend on Wi-Fi/connectivity).
- Persisted selection must be robust: invalid/corrupt values must fall back to `default`.
- Gating before unified lifecycle may leave some runtime/lifecycle edges for follow-up (must be documented).
- On-device validation is required for real memory/service behavior.

## Open questions
- None.

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus).
- 2026-11-16 (clarify-feature, agent): reviewed `docs/README.md`, consolidated plan R44–R56, `main/main.c` fixed boot sequence, and key component APIs (`wifi` init/shutdown/connect, `http_server` init/stop, `network` init-only diagnostics/HTTP worker, `bluetooth` init/enable/disable, `text_console` init/deinit/render task/stdout hook, `shell` init/feed/cwd). Identified spec gaps and asked clarification questions.
- 2026-11-16 (clarify-feature, user): resolved v1 scope (profiles `default` + `low-memory-offline`, boot-only, NVS + `boot_profile` get/set/clear, fallback to `default`, local recovery in all v1 profiles, recommended low-memory matrix, qualitative+measured evidence, gated-init boundary allowed before full unified lifecycle).
- 2026-11-16 (clarify-feature, user): explicitly approved spec as drafted; marked Ready and moved to `docs/plan/ready/boot-profiles-feature-modes.md`. No implementation/validation run in this clarification step.
