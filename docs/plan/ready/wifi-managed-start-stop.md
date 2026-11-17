# Managed Wi-Fi start and stop lifecycle

- **ID:** wifi-managed-start-stop
- **Status:** ready (approved)
- **Created:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `main/main.c:134`, `components/wifi/src/wifi.c:57-140`, `components/wifi/include/wifi.h:15-25`. Related: `docs/plan/ready/unified-service-lifecycle.md`, `docs/plan/ready/optional-http-websocket-service.md`, `docs/plan/consolidated-improvement-plan.md`.
- **Approval:** Approved by user in clarify-feature session, 2026-11-16; pre-approved for implementation/PR flow (merge remains human).

## Summary
Wi-Fi is initialized and started automatically at boot (AP or AP+STA behavior), but teardown is incomplete and not used as a managed lifecycle. This prevents treating Wi-Fi as an optional service whose memory/resources can be reclaimed when offline operation is sufficient.

## Decisions (clarification, 2026-11-16)
- **Boot default / sequence (resolved):** preserve current default UX by auto-starting Wi-Fi (AP-only or AP+STA with auto-connect when stored credentials exist) **only when effective state allows it**. Read boot profile and persisted Wi-Fi enabled/disabled state **before** starting radio/AP/STA/netif bring-up; if profile disallows Wi-Fi or persisted state is disabled in an allowed profile, keep Wi-Fi off for that boot with **no unintended AP/STA/radio bring-up**.
- **Precedence (resolved, 2026-11-16 review):** **Boot profile is highest priority at boot** (see `docs/plan/ready/boot-profiles-feature-modes.md`). Effective behavior: **`effective = profile allows Wi-Fi ? persisted enabled/disabled state : forced disabled/off`** regardless of persisted toggle. In `low-memory-offline`, Wi‑Fi remains disabled at boot even if persisted enabled; in `default`, boot honors persisted enabled/disabled state. `wifi status`/`boot_profile` output must make forced-off-by-profile vs user-persisted-disabled state distinguishable when feasible; document effective behavior in boot/profile docs and Wi-Fi help/status text.
- **AP availability / memory goal:** Wi-Fi (including AP and STA) **can be fully stopped/disabled** when requested (subject to profile precedence). When persisted-disabled or runtime-stopped in an allowed profile, Wi‑Fi service is off until explicitly started; local access over Wi‑Fi is unavailable but memory/power savings are possible.
- **Teardown depth:** managed `stop` means radio down/disconnect with **best-effort cleanup** of safely reclaimable runtime resources. Full `esp_wifi_deinit`/netif/event-loop teardown is **not required**; ESP-IDF/shared-subsystem limitations must be documented.
- **Runtime control / persistence:** add shell commands `wifi start`, `wifi stop`, `wifi status` (or equivalent) and **persist enabled/disabled state** across reboot. `start`/`stop` update persisted state so boot follows user intent within allowed profiles.
- **Stopped-state / restart semantics (resolved, 2026-11-16 review):** while stopped/disabled, operations that require active Wi-Fi (`connect`, `scan`, `disconnect`, `restart`, and other Wi-Fi-dependent operations as applicable) must **fail/deny with clear errors/status** (no auto-start, no implicit side effects). `wifi restart` is allowed **only when Wi-Fi is active**; it preserves persisted enabled state and performs an active-service restart/reconnect cycle (no persisted-intent change). Existing subcommands remain available when Wi-Fi is active except intentional lifecycle additions.
- **Dependencies:** no automatic side-effects. Stopping Wi-Fi must not auto-stop/start dependent services; dependent network operations fail clearly when Wi-Fi/connectivity is unavailable. Deeper coordinated dependent-service policy belongs with `unified-service-lifecycle` / dependent items.
- **Repeated cycles:** in scope — repeated start/stop (and reconnect where applicable) transitions must work without leaks, crashes, or broken event handling/connectivity.
- **Unified lifecycle alignment:** implement Wi-Fi-specific lifecycle now (not blocked on `unified-service-lifecycle`), with clean APIs/hooks/behavior aligned/adapter-ready for a future unified service manager.

## Scope
- Define and implement managed Wi-Fi lifecycle operations for this item: `start`, `stop`, `status`, and `restart` (active-only) semantics, integrated with current init/boot flow as needed and aligned with `unified-service-lifecycle` conventions where practical.
- Keep current boot-time auto-start default UX, but **determine effective state before radio/AP/STA bring-up** and honor boot-profile precedence + persisted enabled/disabled state (forced-off or persisted-disabled => Wi-Fi remains off for that boot with no unintended bring-up).
- Add shell controls: `wifi start`, `wifi stop`, `wifi status` (or equivalent), persisting enabled/disabled state; keep existing `connect`, `disconnect`, `scan` behavior when active, allow `restart` only when active (preserves persisted enabled intent, restarts/reconnects active service), and reject clearly while stopped/disabled/forced-off.
- Implement `stop` as radio-down/disconnect plus best-effort safe cleanup/release of resources that are safely reclaimable (connection/radio state, handler/instance ownership/state reset, internal runtime state as applicable). Document what remains allocated/shared due to ESP-IDF limits.
- Preserve required behavior when Wi-Fi is active: AP/APSTA behavior per current design/stored credentials, STA connect, scan, stored credentials, status reporting, and predictable reconnect semantics.
- Ensure repeated start/stop/reconnect/restart cycles reinitialize required state correctly (event handling, netifs/driver state usage, command behavior) without leaks or stale callbacks.
- Provide clear error/status reporting for stopped/disabled/forced-off-by-profile state and Wi-Fi-dependent operations; no auto-start of Wi-Fi and no auto stop/start of dependent services.

## Non-goals
- Full `esp_wifi_deinit` / netif / default event-loop teardown or guaranteed reclamation of all Wi-Fi/ESP-IDF memory (best-effort `stop` only).
- New Wi-Fi features/protocols unrelated to lifecycle and memory (e.g. provisioning flows, new auth modes, AP config UI changes beyond necessary status/behavior clarity).
- Automatic orchestration/start/stop of dependent services (HTTP server, WebSocket, network diagnostics) — except clear failure/status behavior; policy/coordination tracked in dependent lifecycle items.
- Bluetooth coexistence tuning except identifying/documenting constraints.
- Boot profiles/feature modes beyond persisted Wi-Fi enabled/disabled setting (`boot-profiles-feature-modes`).

## Acceptance criteria
- [ ] Wi-Fi exposes a defined managed lifecycle for this item with at least `start`, `stop`, `status`, and active-only `restart` semantics; `stop` performs radio-down/disconnect and best-effort cleanup/release of safely reclaimable resources.
- [ ] Boot determines effective state **before** radio/AP/STA bring-up and applies precedence: if profile disallows Wi-Fi it remains forced off regardless of persisted state (no unintended bring-up); if profile allows Wi-Fi it honors persisted enabled/disabled state (disabled => Wi-Fi/AP/STA remain off until explicitly started). Status/output distinguishes forced-off-by-profile from user-persisted-disabled where feasible.
- [ ] Shell commands `wifi start`, `wifi stop`, `wifi status` (or equivalent) exist; `start`/`stop` persist enabled/disabled state across reboot. `wifi restart` is allowed only when Wi-Fi is active, preserves persisted enabled state, and does not change persisted intent.
- [ ] While stopped/disabled/forced-off, `wifi connect|disconnect|scan|restart` (and other Wi-Fi-requiring operations as applicable) fail/deny with clear errors/status and do not auto-start Wi-Fi or cause unintended bring-up.
- [ ] Repeated start/stop/restart and reconnect/connect/scan flows (when active) do not leak resources, leave stale handlers/state, or break event handling/connectivity.
- [ ] When active, existing Wi-Fi capabilities remain correct: AP/APSTA behavior, STA connect/disconnect, scan, stored credentials handling, status reporting, and restart/reconnect behavior.
- [ ] ESP-IDF/shared-resource teardown limitations and expected memory-reclaim boundaries are documented.
- [ ] Implementation is adapter-ready/aligned with `unified-service-lifecycle` conventions where practical (clean API boundaries/hooks), without blocking on that item’s unresolved shared design.
- [ ] Automated checks required by `AGENTS.md` for affected scope pass (`make format-check`, `make test`; plus firmware build/lint/integration checks as feasible/required by scope).

## Validation plan
- **Automated (required, report evidence):** `make format-check`, `make test`. For firmware/network changes, run `make build` and `make lint` when feasible in an ESP-IDF v6.1 environment; run relevant integration tests if scope affects integration-tested behavior.
- **Hardware/manual (required):**
  - Boot default case: in a profile that allows Wi-Fi with no prior "disabled" persisted state (or default state), Wi-Fi auto-starts as today (AP-only/AP+STA per stored credentials).
  - Persisted-disabled boot: in a profile that allows Wi-Fi, after `wifi stop`/disabled persisted, reboot keeps Wi-Fi/AP/STA off with no unintended bring-up; `wifi status` and rejected commands report clear stopped/disabled state.
  - Profile-forced-off case: in `low-memory-offline` (or any profile disallowing Wi-Fi), Wi-Fi remains forced off regardless of persisted enabled state with no unintended bring-up; status distinguishes forced-off-by-profile from user-disabled where feasible.
  - Start/enable: `wifi start` enables and persists; AP/STA/connect/scan/status work as expected in allowed profiles.
  - Restart (active-only): when active, `wifi restart` preserves persisted enabled state and restarts/reconnects without changing intent; while stopped/forced-off it fails clearly with no auto-start/bring-up.
  - Stop/disable: `wifi stop` stops radio/AP/STA, persists disabled state, and dependent Wi-Fi operations fail clearly (no auto-start). Observe memory/power-relevant effect where measurable.
  - Repeated cycles: multiple start/stop/restart and connect/disconnect/scan sequences without leaks/crashes/broken events.
  - Record ESP-IDF/shared-resource limitations and any resources not reclaimed by best-effort stop. Reuse `service-memory-budget-observability` `svc_mem`/A-B evidence checklist where useful.

## Expected areas/files
- `components/wifi/src/wifi.c` — lifecycle state, start/stop implementation, handler/instance ownership, best-effort cleanup/reset for repeated cycles
- `components/wifi/include/wifi.h` — public lifecycle API additions/semantics (`start`/`stop`/`status`/enabled-state helpers as chosen)
- `components/wifi/src/wifi_cmd.c` — `start|stop|status` commands, persisted-state updates, rejection behavior while stopped
- `main/main.c` — boot path honors persisted enabled/disabled state while preserving current default auto-start
- Wi-Fi persistence/config storage (NVS namespace/key via existing patterns or wifi component helper)
- Coordination touchpoints only as needed for clear errors/status: `components/network/*`, `components/http_server/*`, `components/websocket/*`
- Docs updates for shell commands, persisted behavior, and teardown limitations (relevant `docs/`/README/help text as applicable)

## Dependencies / prerequisites
- Align with [`./unified-service-lifecycle.md`](./unified-service-lifecycle.md) conventions where practical, but **not blocked** on that item’s shared design (**adapter-ready** implementation allowed now).
- Coordinate expectations with [`./optional-http-websocket-service.md`](./optional-http-websocket-service.md) and optional network diagnostics lifecycle items (no auto side-effects; clear dependency failures).
- [`./boot-profiles-feature-modes.md`](./boot-profiles-feature-modes.md) remains separate; this item covers only persisted Wi-Fi enabled/disabled state.

## Implementation order / prerequisites (2026-11-16, clarified)
- May proceed **now** as Wi-Fi-specific lifecycle/persistence work.
- Expose clean lifecycle APIs/hooks/state/capability reporting so the service can be represented as a unified-lifecycle descriptor/adapter when that framework is integrated (no divergent contract).

## Risks / constraints
- ESP-IDF Wi-Fi/netif/default event-loop init/deinit ordering is restrictive; full teardown/reclaim may be unsafe or unsupported. Best-effort `stop` must avoid corrupting shared/global state.
- Handler instances, netifs, and reconnect/event-group state must be owned/reset correctly to support repeated cycles.
- Stopping Wi-Fi disables AP and STA connectivity; local web/console over Wi-Fi becomes unavailable while stopped (intentional per decision).
- Boot sequencing must evaluate profile/persisted effective state before radio/AP/STA bring-up to avoid unintended bring-up on forced-off or persisted-disabled boots.
- Potential BLE coexistence interactions and memory pressure require hardware measurement/evidence.

## Open questions
- None.

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus).
- 2026-11-16 (clarification, user): keep boot auto-start default; Wi-Fi/AP may be fully stopped/disabled for memory/power savings; `stop` = radio down/disconnect with best-effort cleanup (full deinit not required, document limits); shell `wifi start|stop|status` with persisted enabled state; while stopped reject `connect/scan/disconnect/restart` and Wi-Fi-dependent ops with clear errors (no auto side-effects/dependent orchestration); repeated start/stop cycles in scope; implement now, adapter-ready for `unified-service-lifecycle`; explicitly approved as ready and pre-approved for implementation/PR flow.
- 2026-11-16: moved from `docs/plan/needs-clarification/` to `docs/plan/ready/`.
- 2026-11-16 (review follow-up 2): specified boot sequence (determine effective state before radio/AP/STA bring-up, no unintended bring-up when forced-off/persisted-disabled) and active-only `wifi restart` semantics (preserves persisted enabled intent, no intent change); acceptance/validation updated. No implementation/validation yet.
- 2026-11-17 (implementation, `feat/wifi-managed-start-stop`, in progress): added managed lifecycle API/state in `components/wifi/include/wifi.h` and `components/wifi/src/wifi.c` (`wifi_start/stop/restart/status/state/helpers`, persisted user enabled state in NVS namespace `wifi` key `enabled` as blob, boot-profile forced-off detection via NVS `boot/profile=low-memory-offline` for precedence compatibility). Boot init now determines effective state before radio/AP/STA bring-up (auto-starts only when allowed and user-enabled; otherwise keeps service stopped with no unintended bring-up). `wifi_connect/disconnect/scan/restart` reject while stopped/forced-off with clear errors (no auto-start). `wifi scan` in active AP-only mode temporarily enables AP+STA for scan and restores mode. `wifi_shutdown` performs best-effort managed stop while keeping driver reusable; full `esp_wifi_deinit`/netif/event-loop teardown not performed (documented limitation). Shell command updated in `components/wifi/src/wifi_cmd.c` to support `status|start|stop|connect|disconnect|scan|restart` with persisted start/stop and explicit forced-off/user-disabled messaging. README command summary updated. Host tests updated in `test/test_wifi.c` (idempotent init/reset helper) to match new semantics.
- 2026-11-17 (independent `reviewer` subagent, addressed): fixed High start/connect-failure state divergence by marking service started after successful radio/AP bring-up and treating STA connect-initiation failure as non-fatal (service remains started/AP available). Fixed Medium `wifi stop` forced-off behavior to deny without persisting user-disabled intent (preserves intent across profile changes). Hardened boot-profile precedence parsing to accept blob/string-stored `boot/profile` values for compatibility (`low-memory-offline`, with/without trailing NUL); canonical contract alignment with future `boot-profiles-feature-modes` remains a follow-up dependency. Added host tests for persisted-disabled boot/no bring-up, forced-off precedence/no intent mutation/rejected ops, start/stop persistence, active-only restart, and lifecycle command basics.
- 2026-11-17 (local validation, post-fix): `make format-check` — **pass** (exit 0). `make test` — **pass** (CTest 26/26 passed, Python `scripts` unittest 3/3 OK). `make docs-check` — **pass** (exit 0). `make build` / `make lint` — **not run** (ESP-IDF v6.1 / clang-tidy environment not verified in this session). Hardware/manual validation (boot default/persisted-disabled/profile-forced-off, start/stop/restart/connect/scan cycles, memory evidence) — **not run** (no device). Item remains in `ready/` pending delivery/PR CI green/finalization per workflow.
- 2026-11-17 (review/CI follow-up, `feat/wifi-managed-start-stop` PR #45): renamed public status helper `wifi_is_started` → `wifi_is_service_started` to avoid ESP-IDF `libnet80211` link symbol collision that broke Build & Lint/integration firmware builds (updated header/impl/cmd/tests). Addressed review feedback: `wifi restart` now performs an active service stop/start cycle so healthy AP-only service (no stored STA credentials) restarts successfully without `esp_wifi_connect`; partial `wifi_ensure_driver_initialized` failures perform best-effort cleanup/reset of local driver state for clean retries; `wifi start`/`wifi stop` persist intent aligned with the actual radio transition and roll back persisted intent on failure (while keeping no-op/idempotent cases consistent). Host mocks extended (`vEventGroupDelete`, `esp_event_handler_instance_unregister`, `esp_wifi_deinit`) and tests adjusted for AP-only restart semantics.
- 2026-11-17 (local validation, review/CI follow-up): `make format` — **pass** (exit 0). `make format-check` — **pass** (exit 0). `make test` — **pass** (CTest 26/26 passed, Python `scripts` unittest 3/3 OK). `make docs-check` — **pass** (exit 0). `make build` / `make lint` — **not run** (ESP-IDF v6.1 / clang-tidy environment not verified in this session). Hardware/manual validation — **not run** (no device; remains blocking per PR). CI rerun pending after push.
