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

**Milestone/implementation note (2026-11-16, clarified):** lifecycle-first gate applies (**hard gate**). Unified-lifecycle contract/interfaces/helpers must be landed before profile-gating implementation; this item must not introduce or keep a temporary/parallel ad-hoc boot/service-gating model in `main.c`. Wi-Fi lifecycle work may proceed adapter-ready in parallel, but profile gating must consume unified states/capabilities once landed. Keep console semantics/evidence aligned with `text-console-memory-lifecycle` and `service-memory-budget-observability` as stated below.

## Scope (v1)
- Define two profiles: `default` and `low-memory-offline`.
- Persist selected profile in NVS; provide a shell command to get/set/clear it. Changes apply on **next boot/restart** only.
- Gate boot-time init/start/wiring per profile through **landed** `unified-service-lifecycle` descriptors/interfaces/helpers only (no temporary or parallel ad-hoc boot-gating model in `main.c`).
- Keep a local recovery/debug path in every v1 profile.
- Document per-profile service availability and record measured memory evidence (qualitative acceptance; no guessed hard KB targets).

## Non-goals (v1)
- Runtime profile switching, hot stop/start of services after boot, or general runtime reconfiguration.
- Non-interactive ultra-minimal profiles (deferred).
- Automatic boot-failure rollback/self-healing beyond fallback on missing/invalid/corrupt stored value.
- UI redesign beyond necessary status/output for the profile command.
- Changing safety-critical hardware bring-up.
- Implementing or redesigning the full `unified-service-lifecycle` framework inside this item (this item consumes the landed shared lifecycle contract/interfaces/helpers; it does not own framework design).
- A temporary/parallel ad-hoc boot/service-gating path in `main.c` as a migration shortcut (lifecycle-first gate applies).

## Profile definitions (approved)

### `default`
- **Intent:** preserve current behavior as far as dependencies allow.
- **Services/init:** same boot sequence and wiring as current `main/app_main` (display, calibration, light sensor, RGB LED, brightness, text console, I2C bus, filesystem, Wi-Fi, network, Bluetooth, time sync, HTTP server, WebSocket, system, shell; dependent wiring such as keyboard routing, console output broadcast, WebSocket text input as today).

### `low-memory-offline`
- **Intent:** reduce memory/task/network overhead for offline/local use while keeping local recovery.
- **Enabled/kept (recommended matrix):** display, calibration, light sensor, RGB LED, brightness, text console, I2C bus, filesystem, system, shell, Berry runtime/autoexec (and local recovery commands/scripts including profile selection). Safety-critical/core bring-up remains unchanged.
- **Initialized but forced off by profile:** **Bluetooth** — call `bluetooth_init()` in all profiles (registers `bt`, keeps lightweight state and compatible API/command contract as today), but in `low-memory-offline` force Bluetooth disabled for that boot (no stack enable, no scan/connect/auto-connect). Stored NVS auto-enable value must be preserved and must not be cleared by profile gating; when profile allows Bluetooth (`default`), stored auto-enable behavior applies as today. `bt` remains available for status and local recovery visibility; enable attempts while profile forces Bluetooth off must fail or be rejected with a clear "disabled by boot profile" error and must not start the BT stack.
- **Disabled/unavailable in this profile:** **Wi-Fi** (`wifi` lifecycle forced off by profile per `wifi-managed-start-stop` precedence), **network diagnostics/HTTP client worker** (diagnostics service unavailable/disabled-by-profile; no `network_init` worker ensure and no lazy resource creation in this profile), **time sync** (`time_sync_init`), **HTTP server** (`http_server_init`), **WebSocket** (`websocket_init`) and dependent wiring (network/server routes, console output broadcast to WebSocket, WebSocket text input). No auto-connect/network/BT/server behavior.
- **Unavailable behavior (expected):** Wi-Fi/HTTP/network/time-sync/diagnostics commands/features fail or are unavailable as appropriate for disabled-by-profile/uninitialized services (explicit states/errors; `ping`/`resolve` unavailable in this profile, no diagnostics task/queue/semaphore allocation). Bluetooth stack/radio operations unavailable while forced off by profile, but `bt` command/compatibility surface remains as specified above. Remote console/input over WebSocket unavailable. Serial/on-device local recovery remains.

## Selection, persistence, and command contract (approved)
- **Mechanism:** NVS-persisted profile value read at boot and applied during startup.
- **NVS (concrete, 2026-11-16):** namespace `boot`, key `profile`, string values `default` or `low-memory-offline`. Empty/missing/invalid/corrupt value => treat as `default` (safe fallback).
- **Precedence (resolved, 2026-11-16 review):** **Boot profile is highest priority at boot.** It selects the allowed service set and wiring for that boot. Effective behavior: **`effective = profile allows service ? persisted/service setting (if defined by that service item) : forced disabled/off`** regardless of any persisted toggle. In `low-memory-offline`, Wi‑Fi, network diagnostics, Bluetooth stack/radio, HTTP server, WebSocket, and time sync are forced off/unavailable for that boot even if a persisted "enabled" toggle exists (Bluetooth `bluetooth_init()` still runs in a forced-off mode per profile definition above; stored BT auto-enable value preserved). In `default`, persisted toggles may be honored only where the service plan explicitly defines them. `boot_profile` and affected service status/output must make forced-off-by-profile vs user-disabled/stored state distinguishable when feasible; document effective behavior in this plan and in each affected service plan.
- **Command:** `boot_profile` (shell command)
  - `boot_profile` — print stored raw value (or `default`/none), effective profile for the current boot, effective profile/intent for next boot, and supported names.
  - `boot_profile set <profile>` — persist `default` or `low-memory-offline` to NVS namespace `boot`, key `profile`; report that it takes effect on next boot/restart.
  - `boot_profile clear` — remove stored selection from NVS; equivalent to `default` on next boot.
  - Invalid profile name: return/report error, do not change stored value.
- **Fallback:** missing, invalid, or corrupt stored value => treat as `default` on boot (safe fallback). No automatic boot-failure rollback.

## Recovery/debug contract (v1)
- **Required in every v1 profile (concrete):**
  - **Serial recovery:** UART/USB serial shell input/output remains functional and can run recovery commands/scripts (including `boot_profile` changes for next boot).
  - **On-device console recovery:** the on-device text console is initialized and available for local recovery. When the console is visible/not app-owned, it is rendered and interactive as expected. When hidden/app-owned, it follows [`../done/text-console-memory-lifecycle.md`](../done/text-console-memory-lifecycle.md) semantics (render/touch suspended, UART/stdout forwarding and output listener callbacks active, resident base buffer continues to be updated, heap scrollback not preserved across hide); recovery in that state relies on serial shell and/or returning console visibility/app ownership as applicable.
  - **Other:** filesystem access remains available. Keyboard/input routing remains as available for initialized components.
- Console lifecycle/visibility semantics must remain consistent with [`../done/text-console-memory-lifecycle.md`](../done/text-console-memory-lifecycle.md) (source of truth).
- **Not in v1:** profiles that omit shell/console/UI or rely on rebuild/reflash/external-only recovery.

## Acceptance criteria / definition of done
- [ ] `default` profile preserves current startup behavior and service availability (subject to existing hardware/environment conditions).
- [ ] `low-memory-offline` applies the agreed matrix: Wi-Fi/network diagnostics/HTTP server/WebSocket/time sync are unavailable/disabled by profile as specified; Bluetooth `bluetooth_init()` runs but BT stack/radio is forced off for that boot with stored auto-enable preserved and `bt` behavior compatible with the profile contract; enabled recovery/core/local-runtime behavior works.
- [ ] `boot_profile` supports get/set/clear with correct persistence (NVS namespace `boot`, key `profile`, values `default`/`low-memory-offline`), user-visible output/errors (including stored/effective/next-boot context where specified), and "takes effect on next boot" semantics.
- [ ] Missing/invalid/corrupt persisted profile value falls back to `default` at boot.
- [ ] Local recovery path (serial shell + on-device console + filesystem + profile command) works in both profiles.
- [ ] Documentation (and/or spec notes) describes per-profile feature availability, forced-off-by-profile vs user-disabled effective behavior where applicable, and memory/feature tradeoffs; measured memory evidence is recorded for comparison (no guessed hard KB targets).
- [ ] Automated checks required by `AGENTS.md` for affected scope pass. On-device/manual validation is required when hardware/environment is available; if unavailable, record concrete blockers and what remains unverified (do not claim hardware pass).

## Validation plan
- **Automated (affected scope):** `make format-check`, `make test`; run `make build` and `make lint` when ESP-IDF/clang-tidy environment is available (firmware/build/lint-covered changes). Add/update host/unit tests where feasible (e.g. profile parsing/selection/fallback logic if extracted/testable). Follow `AGENTS.md` reporting rules.
- **Hardware/manual (required when hardware/environment available):** on device, for each profile: set/clear via `boot_profile`, reboot, verify stored/effective profile and service availability/unavailability (including Wi-Fi/HTTP/WS/time-sync/diagnostics disabled-by-profile and Bluetooth forced-off-with-`bt` behavior in `low-memory-offline`), verify serial shell + on-device console recovery path, and capture memory evidence (e.g. `mem`, `berry profile`/heap/watermarks/task-related indicators as available) compared to `default` baseline in comparable conditions. Record results/evidence. If hardware/environment is unavailable, report concrete blockers and unverified items per `AGENTS.md` (do not claim pass).

## Expected areas/files (rough)
- `main/main.c` (boot-time profile gating/wiring)
- New or existing config/profile helper (NVS read/write/validate/fallback), likely under a suitable component or `main/`
- `components/shell/` or shell command registration for `boot_profile`
- `components/system/` and/or docs/help text if command discoverability/output is surfaced there
- `docs/plan/consolidated-improvement-plan.md` (cross-link/status consistency when applicable)
- Possibly component init guards/docs depending on implementation details

## Dependencies / prerequisites
- **Prerequisite:** [`./unified-service-lifecycle.md`](./unified-service-lifecycle.md) (ready, shared lifecycle contract/base). `unified-service-lifecycle` descriptors/types/states/capabilities/boot helpers must land first; boot-profile gating is implemented on descriptor-driven boot/control (no temporary/parallel permanent boot-gating model in `main.c`).
- Interacts with service-specific lifecycle behavior: HTTP/WebSocket (`./optional-http-websocket-service.md`), network diagnostics, Wi-Fi (`./wifi-managed-start-stop.md`), console (`../done/text-console-memory-lifecycle.md`, source of truth for console lifecycle semantics), shell, Bluetooth/time sync (as applicable to disabled/kept services).
- `service-memory-budget-observability` (`./service-memory-budget-observability.md`) is useful for evidence-based comparison but v1 acceptance relies on qualitative availability plus measured evidence (no hard targets).

## Implementation order / prerequisites (resolved, 2026-11-16 review)
- **Lifecycle first (hard gate):** `unified-service-lifecycle` descriptors/types/states/capabilities/boot helpers must land before boot-profile service-gating implementation. Profile gating must use unified-lifecycle interfaces/hooks/descriptor-driven boot control; do not introduce or keep a temporary or parallel ad-hoc boot/service model in `main.c`.
- **Semantic prerequisites (must stay consistent):** `text-console-memory-lifecycle.md` is the source of truth for console lifecycle/visibility/ownership semantics and recovery guarantees referenced by this plan; profile behavior/docs/evidence must follow its definitions (no divergent hidden/app-owned assumptions). `service-memory-budget-observability.md` provides the preferred/canonical memory-evidence template and shared diagnostics vocabulary: record per-profile `svc_mem`/A-B comparisons and state/metric reporting consistent with its contract/checklists when available (or record concrete `n/a`/blocker and reason, without inventing values).
- **Service coordination:** coordinate with `optional-http-websocket-service` and `wifi-managed-start-stop` so disabled/stopped/forced-off-by-profile services and recovery guarantees are represented consistently via unified states/capabilities and effective-precedence rules.
- **Evidence expectation:** per-profile validation must capture both service availability/unavailability and measured memory comparison aligned with observability checklists/vocabulary (when tooling/contract available), in addition to recovery checks.

## Milestones / progress tracking (explicit)
- [ ] **M1 — Prerequisites landed:** `unified-service-lifecycle` contract/interfaces/helpers available; console/observability semantics and vocabulary references confirmed for implementation/evidence.
- [ ] **M2 — Profile persistence/command:** `boot_profile` get/set/clear, NVS storage/fallback, and user-visible stored/effective/next-boot behavior implemented and tested where feasible.
- [ ] **M3 — Descriptor-driven boot gating:** `default`/`low-memory-offline` matrices applied via unified-lifecycle boot/control (no ad-hoc gate), including precedence and recovery guarantees.
- [ ] **M4 — Validation/evidence:** automated checks and (when available) hardware/manual per-profile availability + memory evidence recorded per AGENTS and observability/console prerequisites.

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
- 2026-11-16 (review follow-up): addressed plan-review findings — aligned `low-memory-offline` to run `bluetooth_init()` but force BT off (preserve stored NVS auto-enable, keep `bt` contract), made network diagnostics explicitly unavailable/disabled-by-profile with no lazy creation, set concrete NVS persistence (namespace `boot`, key `profile`, values `default`/`low-memory-offline`), and clarified hardware-validation reporting.
- 2026-11-16 (review follow-up 2): made `text-console-memory-lifecycle` and `service-memory-budget-observability` explicit semantic/evidence prerequisites for profile behavior and per-profile measurements (canonical console semantics and `svc_mem`/A-B/vocabulary alignment when available). No implementation/validation yet.
- 2026-11-16 (review follow-up 3): resolved non-goal/scope contradiction by enforcing lifecycle-first hard gate (removed "v1 allows gated init"/ad-hoc `main.c` gating allowance) and added explicit milestones for tracking. No implementation/validation yet.

