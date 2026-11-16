# Optional HTTP server and WebSocket service control

- **ID:** optional-http-websocket-service
- **Status:** ready (approved)
- **Created:** 2026-11-16
- **Updated:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `main/main.c:139-175`, `components/http_server/src/http_server.c:51-80`, `components/websocket/src/websocket.c:17-22`, `components/websocket/src/websocket.c:253-292`. Related: [`docs/plan/ready/unified-service-lifecycle.md`](./unified-service-lifecycle.md) (shared lifecycle contract, boot/internal descriptors/wrappers, best-effort stop semantics), [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md).
- **Approval:** Approved by user in clarify-feature conversation on 2026-11-16 ("Approve and move to ready"). Pre-approved for implementation/PR flow (merge remains human).
- **Open questions:** none

## Summary
HTTP server and WebSocket currently start on every boot (`main/main.c`), consuming task/stack, socket, and state memory even when remote web/console features are unused. This item makes them optional, lifecycle-controlled services that can be started/stopped safely (best-effort) with clear semantics, while preserving current default behavior unless explicitly disabled by boot/profile/lifecycle control.

## Scope
- **Services/granularity:** treat **HTTP server** and **WebSocket** as **independent** lifecycle-managed services. WebSocket depends on HTTP server availability (`/ws` endpoint registered on `httpd` handle) and cannot be started without a running HTTP server; HTTP may run without WebSocket when WebSocket is stopped/disabled.
- **Default behavior (no explicit override):** **always-on**. Preserve current compatibility: on boot, start HTTP server and then WebSocket when HTTP init succeeds (same effective default as today), unless a boot profile or unified lifecycle descriptor explicitly disables/does not start them.
- **Control surface:** integrate with **unified-service-lifecycle** boot/internal descriptor/profile control and provide **explicit shell command control** aligned with existing commands:
  - `server`: keep/extend for HTTP server lifecycle/status (`server status|start|stop` or equivalent), compatible with current behavior where possible.
  - `ws`: extend/align for WebSocket lifecycle/status (not only client count) so WebSocket can be started/stopped independently when HTTP is available; command registration/lifecycle must be safe across start/stop cycles.
  - No new generic runtime service-manager command in this item. No persisted runtime enable setting required in v1 unless added as a clearly specified follow-up (default remains always-on).
- **Start/stop behavior:** implement lifecycle hooks/adapters compatible with the unified contract.
  - **Start:** idempotent/safe when already running (report status/no-op or documented behavior). Re-register endpoints/handlers/listeners only as needed and avoid duplicate console command registration across restarts.
  - **Stop (best-effort graceful):** follow unified lifecycle best-effort semantics. Stop accepting new work; for active clients/uploads/requests prefer safe refusal/defer when feasible, otherwise close clients/notify and allow in-flight work to fail cleanly or drain briefly as bounded/appropriate, then teardown in safe order (WebSocket before HTTP when both affected). Unregister/disable handlers and listeners where supported (`esp_httpd`/component APIs), close/release COS-owned client-tracking state, clear/reset mutex-guarded state safely, and detach console/input bridges (e.g. WebSocket text handler, console output broadcast/listener wiring) as required to avoid callbacks into stopped services or deadlocks.
  - **Unsupported/unsafe teardown:** if `esp_httpd` or platform operations cannot be completed safely in a given state, report failure/unsupported per capability and document limits (no forced unsafe teardown).
- **Correctness across cycles:** when enabled/started, preserve existing behavior for HTTP auth, static file serving/API, uploads, and WebSocket console bridging as applicable. Stopping WebSocket must not break an independently running HTTP server; stopping HTTP must also stop/dependently disable WebSocket endpoint/clients.
- **Resource cleanup (completion bar):** **best-effort + documented limits**. Release/cleanup all COS-owned resources that can be safely released (client arrays/state, handlers/listener registrations, mutexes where safe, command/runtime wiring state). Invoke supported ESP-IDF stop/teardown. Do **not** require full heap/task memory reclamation where ESP-IDF/httpd/sockets do not return memory; measure/report impact where feasible and document non-reclaimable resources.

## Non-goals
- New HTTP API endpoints or WebSocket protocol changes unrelated to lifecycle/memory/control.
- TLS/HTTPS feature work.
- Reworking multipart upload parsing/size limits except minimal changes required for lifecycle safety (see `http-upload-memory-bounds`).
- Persisted remembered on/off setting across reboots as a required deliverable of this item (default remains always-on; can be follow-up/profile concern).
- New generic service-manager shell command.
- Guaranteeing full ESP-IDF subsystem memory reclamation after stop.

## Acceptance criteria / definition of done
- [ ] HTTP server and WebSocket are represented/controllable as **independent** lifecycle-managed services with explicit dependency (WebSocket requires running HTTP), aligned with `unified-service-lifecycle` descriptors/capabilities.
- [ ] Default behavior without explicit override remains **always-on** (HTTP+WebSocket started on boot when HTTP init succeeds), matching current behavior unless disabled by boot/profile/lifecycle configuration.
- [ ] Runtime control exists via unified lifecycle/boot-profile integration plus explicit shell commands (`server` and `ws` or equivalent) for start/stop/status as specified; operations are idempotent/safe and do not duplicate command registrations across cycles.
- [ ] Best-effort graceful stop behavior is implemented/documented for active WebSocket clients and in-flight HTTP requests/uploads (safe refusal/defer where feasible; otherwise clean failure/close and ordered teardown), with no callbacks/deadlocks/leaks into stopped services.
- [ ] Stopping releases/cleans all safely releasable COS-owned resources and invokes supported ESP-IDF teardown; non-reclaimable platform memory/resources and known limits are documented. No requirement to fully reclaim heap/task memory.
- [ ] When started/enabled, existing HTTP auth/static/API/upload and WebSocket console bridging behavior remains correct across start/stop/start cycles (subject to dependency/order).
- [ ] Automated checks required by `AGENTS.md` for affected scope pass and are reported with evidence: `make format-check`, `make test`; and when environment is available `make build`, `make lint` (and `make docs-check` if docs/Doxygen scope requires). Host/unit tests added/updated where feasible.
- [ ] Hardware/manual validation performed when feasible on device: start/stop/status commands and boot default behavior, active client/upload stop behavior, and basic memory/log observations; results/limitations documented. If hardware unavailable, state blocker per `AGENTS.md`.

## Validation plan
- **Automated (required):** `make format-check`, `make test`. Run `make build`, `make lint` when ESP-IDF v6.1/clang-tidy environment is available (firmware/network/lifecycle changes). Run `make docs-check` if public headers/Doxygen comments are changed. Add/update host-based unit tests where feasible (e.g. state/idempotency helpers, command parsing, pure logic). Report exact commands and pass/fail/blockers per `AGENTS.md`.
- **Hardware/manual (required when feasible):** on target device verify default always-on boot behavior unchanged; `server`/`ws` start/stop/status and dependency enforcement (ws cannot start without http, stopping http stops/disables ws); HTTP auth/static/API/upload and WebSocket console work after restart cycles; stopping with connected WS client(s) and (if practical) in-flight upload/request behaves per best-effort graceful spec; capture logs and `mem`/heap observations before/after stop to document reclaim/limits. Document any ESP-IDF non-reclaimable resources observed.

## Expected areas/files
- `main/main.c` (boot wiring, dependency-aware start, lifecycle descriptor/adapter integration)
- `components/http_server/src/http_server.c`, `components/http_server/include/http_server.h` (init/start/stop idempotency, state, teardown)
- `components/http_server/src/http_server_cmd.c` (extend `server` command for lifecycle/status)
- `components/http_server/src/http_api.c`, `components/http_server/src/http_upload.c`, `components/http_server/src/http_static.c`, `components/http_server/src/http_auth.c` and related headers as needed (safe re-init/teardown across cycles)
- `components/websocket/src/websocket.c`, `components/websocket/include/websocket.h` (start/stop/deinit hooks, client cleanup, handler/listener detach, `ws` command lifecycle control)
- `components/service_lifecycle/` or unified lifecycle integration points as introduced by `unified-service-lifecycle`
- `docs/plan/consolidated-improvement-plan.md` and related plan docs as needed (status/link consistency)
- `test/` host tests where feasible

## Dependencies / prerequisites
- **Depends on/aligned with:** [`docs/plan/ready/unified-service-lifecycle.md`](./unified-service-lifecycle.md) — must use/align to its static descriptor contract, states/capabilities, explicit dependency/fail-fast start, best-effort stop, and boot/internal integration model. Service-specific deep teardown details beyond minimal contract adaptation may be implemented here for HTTP/WebSocket.
- **Related:** [`docs/plan/ready/boot-profiles-feature-modes.md`](./boot-profiles-feature-modes.md) (boot-time gating of manageable services), [`docs/plan/needs-clarification/http-upload-memory-bounds.md`](../needs-clarification/http-upload-memory-bounds.md), [`docs/plan/needs-clarification/service-memory-budget-observability.md`](../needs-clarification/service-memory-budget-observability.md).
- **Implementation prerequisite decisions (resolved):** default always-on, boot/profile + `server`/`ws` commands, independent services with dependency, best-effort graceful stop, best-effort memory reclaim with documented limits.

## Risks / constraints
- `esp_httpd` stop/start, URI handler lifecycle, and active sockets/requests/uploads can be sensitive to ordering and in-flight operations; unsafe teardown may crash or leak.
- WebSocket client tracking, async send completions, console output listener, and shell input handler integration must not leak callbacks, access freed state, or deadlock mutexes during stop/restart.
- `http_server_set_session_close_cb` is a single callback slot shared by components; lifecycle wiring must preserve correct close handling across stop/start.
- Shell commands (`server`, `ws`) are currently registered during init; restart must avoid duplicate `esp_console_cmd_register` errors and keep command behavior consistent.
- `config.max_open_sockets = 7` and `stack_size = 8192` are fixed `esp_httpd` costs while running; after stop, ESP-IDF may not reclaim all task/heap/socket memory (must be documented, not assumed).
- Resource-constrained ESP32: avoid extra persistent buffers/state; prefer static/minimal bookkeeping.

## Open questions
- None.

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus).
- 2026-11-16 (clarify-feature, agent): reviewed `docs/README.md`, `docs/plan/consolidated-improvement-plan.md`, target item, `docs/plan/ready/unified-service-lifecycle.md`, and current implementation (`main/main.c`, `components/http_server/src/http_server.c`, `components/http_server/src/http_server_cmd.c`, `components/http_server/include/http_server.h`, `components/websocket/src/websocket.c`, `components/websocket/include/websocket.h`).
- 2026-11-16 (clarify-feature, user, round 1): resolved open questions — **default:** always-on (current behavior preserved unless explicitly disabled). **Control:** boot/profile + `server`/`ws` commands (aligned with unified lifecycle; no generic service-manager command). **Granularity:** independent services (WebSocket depends on HTTP). **Stop behavior:** best-effort graceful stop. **Memory reclaim:** best-effort + documented limits (no requirement for full ESP-IDF heap/task reclamation).
- 2026-11-16 (clarify-feature, agent): updated spec in place with resolved scope, acceptance criteria/DoD, validation plan, dependencies, risks, and no remaining open questions. No implementation or validation run.
- 2026-11-16 (clarify-feature, user): explicitly approved specification and requested move to ready. Marked **ready (approved)** and moved to `docs/plan/ready/optional-http-websocket-service.md`; consolidated plan link/status updated. Pre-approved for implementation/PR flow (merge remains human). No implementation or validation run.
