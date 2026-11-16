# Berry runtime and service inspection (v1)

- **ID:** berry-runtime-service-inspection-v1
- **Status:** ready (approved 2026-11-16)
- **Created:** 2026-11-16
- **Source/links:** User request (chat, 2026-11-16). Clarified from [`needs-clarification/berry-runtime-service-inspection-v1.md`](../needs-clarification/berry-runtime-service-inspection-v1.md) (history preserved in git). Related: [`components/berry_lang/include/berry_runtime.h`](../../../components/berry_lang/include/berry_runtime.h), [`components/berry_lang/src/berry_runtime.cpp`](../../../components/berry_lang/src/berry_runtime.cpp), [`components/shell/src/shell_berry_cmd.c`](../../../components/shell/src/shell_berry_cmd.c), [`main/main.c`](../../../main/main.c), [`components/http_server/include/http_server.h`](../../../components/http_server/include/http_server.h), [`components/websocket/include/websocket.h`](../../../components/websocket/include/websocket.h), [`components/wifi/include/wifi.h`](../../../components/wifi/include/wifi.h), [`components/bluetooth/include/bluetooth.h`](../../../components/bluetooth/include/bluetooth.h), [`components/network/include/network.h`](../../../components/network/include/network.h), [`components/time_sync/include/time_sync.h`](../../../components/time_sync/include/time_sync.h). Cross-linked: [`../consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (progress log / queue state), [`./service-memory-budget-observability.md`](./service-memory-budget-observability.md) (complementary `svc_mem` memory/budget snapshot plan; split & linked).
- **Approval:** Explicitly approved as drafted by user in chat (2026-11-16). Pre-approved for implementation/PR/babysit-to-green flow; merge remains human.

## Summary
Provide a lightweight, shell-based inspection slice for Berry runtime and core services on-device. v1 lists the active Berry UI app (or `none`), reports best-effort status for selected core connectivity services, and exposes VM-wide Berry/system memory using existing snapshot sources/labels. Graceful stop of the active UI app remains `berry close` (no new stop command, no force-kill). Requested items deferred from this slice: per-app/per-service memory attribution, service start/stop, service autostart enable/disable persistence, and multi-level/force stop semantics.

**Relationship to `service-memory-budget-observability` (split & linked, 2026-11-16):** this plan owns quick operational inspection (`services`, `berry app`, existing `berry profile`/`mem` semantics). [`service-memory-budget-observability.md`](./service-memory-budget-observability.md) owns the dedicated cross-service memory/budget snapshot command (`svc_mem`) and before/after evidence workflow. Keep command roles/output contracts distinct and cross-referenced.

### Shared collector contract (must fix #3, 2026-11-16)
- **Owner (recommended):** shared read-only collectors/helpers live under the `service-memory-budget-observability` implementation scope (or a clearly named shared module) and are consumed by both `services`/`berry app` inspection and `svc_mem`. Avoid duplicating state/memory gathering logic.
- **Vocabulary:** use identical service IDs/names, state strings, and `n/a`/unknown semantics in `services`, any Berry/app inspection surfaces, and `svc_mem` output. If a new state is needed, add it in one place and document it here and in `service-memory-budget-observability.md`.

## Feasibility (initial validation, codebase review 2026-11-16)

### Berry apps / scripts
- **Runtime model:** Single shared Berry VM (`berry_runtime_init/run_file/open_app/eval/close_app`). UI apps run via `berry_runtime_ui_launch` / `__cos_entry`; runtime tracks one active UI app internally and `berry_runtime_close_app()` closes it. `berry close` exists in shell.
- **"Running apps" semantics (v1):** No multi-process app model or public enumeration API. Command scripts run synchronously in the shared VM and are not treated as "running" after return. v1 lists **active UI app only** (path/state) or `none`.
- **Memory:** `berry_runtime_get_memory_info()` / `berry profile` expose **VM-wide** GC managed bytes, GC step rate, and heap/internal snapshots (R5). No per-app/per-script allocation accounting exists.
- **Stop levels:** Graceful close of active UI app is feasible (`berry_runtime_close_app()`). Force-kill/VM reset/interruption of arbitrary scripts is out of scope and unsafe in shared-VM model.

### Services / features
- **Current state:** Component-specific init/status APIs exist (HTTP server, WebSocket, WiFi, Bluetooth, network, time sync) but no unified registry/lifecycle/autostart manager.
- **v1 reporting:** Best-effort, read-only status from existing APIs; unknown/unavailable states must be labeled explicitly. No per-service memory attribution.
- **Deferred:** Start/stop/restart control, autostart enable/disable persistence, dependency-aware lifecycle management.

### Cross-cutting constraints
- ESP32 RAM/flash/CPU constrained: one-shot inspection only, no polling/continuous "top" loop.
- Inspection must not mutate app/service state except existing `berry close` when explicitly invoked.

## Scope
- **Shell commands (v1):**
  - **`services`** (new): print one-shot status for core services: **HTTP server**, **WebSocket**, **WiFi**, **Bluetooth**, **Network**, **Time sync** (best-effort from existing component APIs; explicit `n/a`/`unknown` where state cannot be determined).
  - **`berry app`** (new subcommand): print active Berry UI app as `path=<virtual_path> type=ui state=running` or `none`. Requires exposing active app path from runtime if not already public (additive, read-only).
  - **`berry profile`** (existing, may be extended minimally): remains the source for Berry VM-wide runtime memory and heap snapshots (`berry_runtime_get_memory_info` / existing labels). Keep existing fields/behavior compatible; may include active app line or document use of `berry app`.
  - **`berry close`** (existing): unchanged; graceful close of active UI app only.
  - **`mem`** (existing): unchanged; system heap snapshot reference.
- **Memory semantics:** VM-wide Berry GC/runtime memory and system/default/internal heap snapshots only, clearly labeled (no per-app/per-service attribution).
- **No new APIs/surfaces:** no Berry/TS SDK, no HTTP/WebSocket endpoints, no frontend UI in this slice.

## Non-goals
- Per-app/per-script or per-service memory attribution/ownership.
- Service start/stop/restart control and service autostart enable/disable persistence/configuration.
- Force-kill/multi-level Berry stop, VM reset controls, or interrupting arbitrary script execution.
- Background/script process manager, multi-app/multi-VM execution model, or listing synchronous command scripts as "running".
- New Berry/TS SDK bindings, HTTP/WebSocket inspection APIs, or frontend UI for this slice.
- Managing/reporting every subsystem beyond the selected core service set.

## Acceptance criteria
- [ ] **Berry app (v1):** `berry app` prints active UI app path/name and state/type (e.g. `path=<vfs_path> type=ui state=running`) when a UI app is active, and prints `none` (or equivalent explicit no-app message) when none is active. Command scripts are not listed. `berry close` remains the graceful stop mechanism for the active UI app (no new stop command/force-kill).
- [ ] **Services:** `services` prints one-shot status for **HTTP server**, **WebSocket**, **WiFi**, **Bluetooth**, **Network**, **Time sync** using best-effort existing component APIs. Each service has clear labels and shows unavailable/unknown/not-initialized states explicitly when not determinable.
  - Proposed reportable states (implementation may adjust labels but must stay truthful): HTTP server `running`/`stopped` (handle/server state); WebSocket `clients=<n>` and/or availability relative to HTTP server; WiFi `connected`/`disconnected`, `stored_credentials=yes|no` as available; Bluetooth `disabled|enabling|enabled`, `connected=yes|no` (+ device name if available/cheap); Network `initialized`/coarse state as available (or explicit `unknown` if no getter); Time sync `synced=yes|no` (+ epoch/local/UTC availability as cheap).
- [ ] **Memory:** Berry VM-wide/runtime memory and system heap snapshots are reported using existing sources/labels (`berry_runtime_get_memory_info` / `berry profile`, `system_get_info` / `mem`) and clearly labeled as VM-wide/system (not per app/service). No per-entity attribution added.
- [ ] **UX/docs:** shell help/usage updated for new/changed commands (`berry` subcommands, `services`); README/help docs updated as needed to reflect inspection commands and limitations.
- [ ] **Quality:** implementation follows repo conventions (C/C++ style, ESP-IDF/component patterns, resource constraints) and passes scope-appropriate validation with recorded evidence. No invented hardware validation; on-device checks recorded when performed.

## Validation plan
- **Automated (required where environment allows, per `AGENTS.md`):** `make format-check`, `make test` for C/C++/host changes. If firmware/shell/runtime components change: `make build` and `make lint` when ESP-IDF/clang-tidy environment is available; `make docs-check` if public headers/docs comments affected. Add/update host unit tests for new command parsing/output helpers and service-status formatting/reporting logic where feasible (mock component APIs). Frontend checks not expected for this shell-only slice.
- **Hardware/manual (CYD, recommended/record as performed):** flash and verify on device: `services` output reflects actual states (HTTP/WebSocket running, WiFi connected/disconnected, Bluetooth enabled/connected as applicable, time sync state), `berry app` shows `none` when idle and active UI app path/state while a Berry UI app is running, `berry close` still closes active UI app gracefully, memory lines consistent with existing `berry profile`/`mem` semantics. Record evidence/limitations in plan/progress; do not claim hardware validation if not executed.
- **Blockers:** if ESP-IDF build/lint or hardware access unavailable, run feasible automated checks and report exact blockers and rerun steps.

## Expected areas/files
- `components/berry_lang/include/berry_runtime.h`, `components/berry_lang/src/berry_runtime.cpp` — expose active UI app path/state getter as needed for `berry app` (minimal, read-only); keep lifecycle/`close_app` semantics unchanged.
- `components/shell/src/shell_berry_cmd.c` (and/or `components/shell/include/shell.h`/`components/shell/src/shell.c` registration) — extend `berry` subcommands/help/usage (`app`, profile integration as decided) and register `services` command (new handler/file if cleaner, e.g. `shell_services_cmd.c`).
- Service status adapters/read-only queries (reuse existing APIs): `components/http_server/*` (`http_server_get_handle`), `components/websocket/*` (`websocket_client_count`), `components/wifi/*` (`wifi_is_connected`, `wifi_has_stored_credentials`), `components/bluetooth/*` (`bluetooth_is_enabled/enabling/connected/...`), `components/network/*`, `components/time_sync/*` (`time_sync_is_synced`), plus any small public getter additions if required.
- `test/` — host unit tests for command parsing/output/service-status formatting where mockable.
- Docs: this plan item, `docs/plan/consolidated-improvement-plan.md` cross-link/status as applicable, `README.md` and shell help text updates. No `frontend/` or `storage/ts`/SDK binding changes expected.

## Dependencies / prerequisites
- None external. Relies on existing component APIs for best-effort status; may require small additive read-only getters (e.g. Berry active app path, network init/state visibility) if not currently exposed.
- **Related (split & linked):** [`service-memory-budget-observability.md`](./service-memory-budget-observability.md) — complementary `svc_mem` plan. Coordinate shared read-only state/memory collectors and consistent service naming/state/`n/a` semantics where feasible; no hard implementation-order dependency implied beyond avoiding duplication.

## Risks / constraints
- ESP32 RAM/flash/CPU limits: inspection must remain lightweight (one-shot queries only; no polling/continuous "top" loop in v1).
- Some service states may be only partially observable via current APIs; output must be truthful (`unknown`/`n/a`/coarse) rather than guessed.
- Active UI app path may not be publicly queryable today; additive runtime API exposure must not change lifecycle behavior.
- `berry close` affects active UI app/console state as today; inspection commands must not mutate service/app state except existing close behavior when explicitly invoked.
- Original request items (service memory attribution, service start/stop, autostart enable/disable, force-kill) are intentionally deferred/out-of-scope for this v1 slice.

## Open questions
- none

## Decisions / resolved questions
- [x] **2026-11-16 (user, round 1): v1 slice** = **Active UI app + service status** (recommended). List active Berry UI app (if any) and selected services with status/state; show VM-wide Berry + system heap snapshots; graceful close of active UI app only. Out of v1: per-app/per-service memory attribution, service start/stop, autostart persistence, force-kill/multi-level stop.
- [x] **2026-11-16 (user, round 2): interface** = **Shell commands** (recommended). No Berry/TS SDK or HTTP/WebSocket API work required for this slice.
- [x] **2026-11-16 (user, round 2): service inventory scope** = **Core connectivity/services** (recommended): HTTP server, WebSocket, WiFi, Bluetooth, Network, Time sync (report best-effort status/state from existing APIs).
- [x] **2026-11-16 (user, round 2): Berry app listing** = **Active UI app path/state or `none`** (recommended). One-shot listing shows active UI app path/name and basic state/type=UI if running, otherwise explicitly none. Command scripts not listed.
- [x] **2026-11-16 (user, round 2): stop behavior** = **Reuse existing `berry close`** (recommended). Graceful close of active UI app only; no new stop command/force-kill.
- [x] **2026-11-16 (user, round 2): memory reporting** = **Reuse existing snapshots/labels** (recommended). VM-wide Berry runtime memory and system heap from existing snapshot sources, clearly labeled (not per app/service).
- [x] **2026-11-16 (user, round 3): command UX** = **New `services` + extend `berry`** (recommended). Add `services` command for core service statuses; extend `berry` for active app/VM-wide memory visibility while keeping `berry close` and `mem`.
- [x] **2026-11-16 (user, round 3): service set** = **HTTP server, WebSocket, WiFi, Bluetooth, Network, Time sync** (recommended), with best-effort states from existing component APIs.
- [x] **2026-11-16 (user, round 4): approval** = **Approved as drafted** and **pre-approved for implementation/PR/babysit-to-green flow** (merge remains human). User in chat, 2026-11-16.

## Progress / evidence
- 2026-11-16: created in `needs-clarification/`. Initial feasibility validation performed via codebase inspection (single shared Berry VM, active UI app lifecycle, VM-wide memory only, component-specific service APIs without unified registry/autostart manager).
- 2026-11-16 (clarify-feature, rounds 1–3): clarified v1 slice (active UI app + core service status, shell commands, `services` + extend `berry`, reuse `berry close`/existing memory snapshots), selected service set (HTTP server, WebSocket, WiFi, Bluetooth, Network, Time sync), and non-goals/deferred items.
- 2026-11-16 (clarify-feature, round 4): concrete spec drafted and **explicitly approved as drafted** by user in chat; pre-approved for implementation/PR flow (merge remains human). Moved to `docs/plan/ready/berry-runtime-service-inspection-v1.md`. No implementation/validation yet.
