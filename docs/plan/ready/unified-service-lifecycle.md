# Unified service lifecycle and teardown control

- **ID:** unified-service-lifecycle
- **Status:** ready (approved)
- **Created:** 2026-11-16
- **Updated:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `main/main.c:105-183`, `components/wifi/include/wifi.h:22-25`, `components/wifi/src/wifi.c:125-140`, `components/http_server/include/http_server.h:15-19`, `components/http_server/src/http_server.c:40-49`, `components/berry_lang/include/berry_runtime.h:23-27`, `components/berry_lang/src/berry_runtime.cpp:2115-2177`, `components/text_console/include/text_console.h:58-64`, `components/text_console/src/text_console.cpp:469-503`. Related/tracker: [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md) (2026-11-16 audit follow-ups). Related items: [`docs/plan/ready/boot-profiles-feature-modes.md`](./boot-profiles-feature-modes.md), [`docs/plan/ready/wifi-managed-start-stop.md`](./wifi-managed-start-stop.md), [`docs/plan/ready/optional-http-websocket-service.md`](./optional-http-websocket-service.md), [`docs/plan/ready/optional-network-diagnostics-worker.md`](./optional-network-diagnostics-worker.md), [`docs/plan/ready/text-console-memory-lifecycle.md`](./text-console-memory-lifecycle.md), [`docs/plan/needs-clarification/shell-input-service-lifecycle.md`](../needs-clarification/shell-input-service-lifecycle.md), [`docs/plan/ready/service-memory-budget-observability.md`](./service-memory-budget-observability.md).
- **Approval:** Approved by user in clarify-feature conversation on 2026-11-16 ("Approve and move to ready"). Pre-approved for implementation/PR flow (merge remains human).
- **Open questions:** none

## Summary
Define and implement a minimal, common lifecycle model for firmware services (static descriptors/registry) so boot sequencing and internal/profile-driven control are predictable and optional services can be started/stopped through one contract. v1 delivers the framework and integrates it into the boot path (`app_main`) with adapters/wrappers for the core boot set, aligning with `boot-profiles-feature-modes` and service-specific lifecycle items. Deep per-service resource-reclaim/teardown work remains primarily owned by the corresponding service items except minimal changes required to satisfy the unified contract.

**Milestone note (2026-11-16):** implement via the ordered delivery slices/gate in this plan; keep slices explicit in PRs (slice 1/2 required before boot-profile and core HTTP/WS lifecycle gating) to reduce `main.c`/boot risk.

## Scope (v1)
- **Design:** lightweight **static service descriptor/table** (no dynamic registration manager). Each descriptor covers at minimum: stable service id/name, dependencies (explicit), lifecycle hooks/capabilities (init/start/stop/deinit or subset), current state/enabled/result reporting, and flags/notes for unsupported stop. Machinery must stay small (ESP32 RAM/flash constraints).
- **New shared component:** add a small shared component/header for the lifecycle model (e.g. `components/service_lifecycle/` or `components/service_manager/` with public header under `include/`) providing descriptor types, ordering/dependency helpers, and boot-run helpers suitable for static tables.
- **Boot integration:** refactor `main/main.c` boot flow to use the unified model (ordered descriptor-driven init/start and wiring), replacing ad-hoc sequencing where in scope while preserving current default behavior, logging, and conditional wiring semantics (e.g. websocket only if http ok; keyboard routing/console broadcast/ws input depend on service availability).
- **Core boot set (v1 integration):** all services/subsystems currently initialized/wired in `app_main` are represented as descriptors/adapters so boot-profiles can gate by service:
  - **Core/always-on (recovery & base bring-up, not stopped in v1 profiles):** NVS init, display, calibration, light sensor, RGB LED, brightness, text console, I2C bus, filesystem, system, shell. (Local recovery path must remain available per `boot-profiles-feature-modes`.)
  - **Manageable/optional (gatable/stoppable via lifecycle when supported):** wifi, network, bluetooth, time_sync, http_server, websocket (depends on http_server). `wifi`, `http_server`/`websocket`, and other service-specific deep teardown/reclaim semantics are specified/implemented by their own items where applicable; v1 provides contract/adapters and best-effort invocation.
- **Services lacking stop/deinit today:** represented with init/start-only (or no-op/unsupported-stop) descriptors and explicit capability/state reporting; no fabricated teardown. Document limitations.
- **Compatibility:** keep existing public APIs (`wifi_init/wifi_shutdown`, `http_server_init/http_server_stop`, `websocket_init`, `berry_runtime_init/berry_runtime_deinit`, `text_console_init/text_console_deinit`, `bluetooth_init`, `network_init`, etc.) as **compatible wrappers/adapters** over/alongside the new model where they are in scope; no breaking API removal in v1.
- **Control surface (v1):** boot/internal and profile-driven control only (usable by `main` and future/ready boot-profile integration). No new user-facing shell commands/config for general runtime service start/stop required in v1 (service-specific commands remain governed by their own items, e.g. `wifi start|stop` in ready wifi item).
- **Documentation:** document the lifecycle model, descriptor/contract, service inventory (core vs manageable), dependency/order, failure/stop semantics, and per-service limitations/non-reclaimable ESP-IDF resources as known.

## Delivery slices (single plan, ordered validation gates)
Keep one plan/contract and implement in ordered slices to reduce boot/integration risk (each slice must pass scope-appropriate validation and preserve behavior parity before proceeding):

1. **Framework + contract + helpers:** shared component/header, descriptor/state/capability types, dependency/order/fail-fast/best-effort-stop helpers and boot-run primitives, plus host/unit tests where feasible.
2. **Boot wiring parity (minimal/core subset):** refactor `main.c` to descriptor-driven boot/init/wiring for a minimal/core subset with preserved default behavior, logging, and conditional wiring semantics; smoke-validate on device when feasible.
3. **Remaining adapters + integration:** represent remaining boot-set services/adapters (including manageable/optional descriptors and unsupported-stop reporting), align existing public APIs as compatible wrappers/adapters, complete docs/cross-links, and validate full default boot/profile-readiness integration as specified.

The contract defined in this plan remains stable across slices. Do not land later dependent work (boot-profile gating, HTTP/WS lifecycle) until the required unified-lifecycle interfaces from this plan have landed per sequencing gate.

## Non-goals (v1)
- Rewriting unrelated business logic inside services.
- Implementing complete, reclaiming stop/deinit for every audited service in this item (owned by `wifi-managed-start-stop`, `optional-http-websocket-service`, `optional-network-diagnostics-worker`, `text-console-memory-lifecycle`, `shell-input-service-lifecycle`, `bluetooth-lazy-scan-state`, etc., except minimal adapter/contract work here).
- New user-facing runtime service-control UI/shell commands beyond boot/internal integration (unless required by a separate approved item).
- Runtime hot profile switching.
- Changing hardware support targets or safety-critical bring-up behavior.
- Guaranteeing full ESP-IDF subsystem memory reclamation where the platform does not support it (best-effort/documented).

## Lifecycle contract (v1 decisions)
- **State/capabilities:** each service has an explicit lifecycle state (at least uninitialized/initialized-or-started/stopped/failed/unsupported-stop as applicable) and reports enablement/availability; failures surfaced via return codes/logging consistent with existing `log_init_result` style.
- **Dependencies/order:** explicit dependency/order declared in descriptors. Boot/start runs in dependency-safe order. Start/init of a service with missing/failed required dependencies is **fail-fast** (skipped/failed with clear log/result; dependents not started as if available).
- **Stop/teardown:** **best-effort**. Stop requests invoke supported stop/deinit hooks; stop in reverse/dependent-safe order where applicable. Unsafe/in-flight/busy cases may be refused or reported per service capability (no forced unsafe teardown in framework). Owned runtime resources should be released when the service/hook supports it; **ESP-IDF/shared-subsystem non-reclaimable memory must be documented**, not assumed freed.
- **No implicit cross-service side-effects:** lifecycle does not auto-start/stop unrelated dependent services except as explicitly defined by dependency/order rules; higher-level orchestration policies belong to specific items/profiles.
- **Concurrency:** v1 targets boot/internal serialized use; no requirement for a general thread-safe runtime service manager beyond what is needed for safe boot/profile use (races/in-flight operations remain a documented risk and may require service-specific handling).

## Acceptance criteria / definition of done
- [ ] A documented, implemented static-descriptor service lifecycle model exists (types/API, states/capabilities, explicit dependencies/order, failure and best-effort stop semantics) in a new shared component/header.
- [ ] `app_main` boot/init/wiring uses the unified model/descriptors (or explicit adapters) for the core boot set instead of only ad-hoc sequencing, preserving current default behavior and availability semantics.
- [ ] Core/always-on vs manageable/optional services are documented; services without stop support are represented as unsupported/no-op with documented limitations.
- [ ] Existing in-scope public init/stop/deinit APIs remain compatible wrappers/adapters where applicable.
- [ ] The model is usable for boot-time gating/profile integration (aligned with `boot-profiles-feature-modes`) without requiring full per-service teardown implementation in this item.
- [ ] Automated checks required by `AGENTS.md` for affected scope pass: `make format-check`, `make test`, and (when environment available) `make build`, `make lint`, `make docs-check`; add/update host/unit tests for registry/order/dependency/failure logic where feasible. Results reported with evidence/blockers per `AGENTS.md`.
- [ ] Hardware/manual validation (when feasible): smoke boot on device confirms default startup behavior remains unchanged. Deep stop/reclaim/memory measurements are deferred to dependent service items and `boot-profiles-feature-modes` validation (documented).

## Validation plan
- **Automated (required, per slice as applicable):** `make format-check`, `make test`. Run `make build`, `make lint`, and `make docs-check` when the ESP-IDF v6.1 / clang-tidy / Doxygen environment is available (firmware/build/lint/docs-covered changes). Add/update host-based unit tests where feasible for descriptor ordering, dependency fail-fast behavior, state transitions, and unsupported-stop handling. Report exact commands and pass/fail/blockers per `AGENTS.md`.
- **Hardware/manual (feasible smoke, especially slice 2+):** if hardware is available, boot device with default configuration/profile and verify startup logs/service availability equivalent to current behavior (no regressions in enabled default path). Record observations.
- **Deferred to related items:** on-device start/stop cycles, per-service resource reclamation measurements, and profile-specific memory evidence are validated under `wifi-managed-start-stop`, `optional-http-websocket-service`, `boot-profiles-feature-modes`, and other service-specific items as specified.

## Expected areas/files
- `main/main.c` (descriptor table/boot orchestration, wiring/adaptation)
- New shared component, e.g. `components/service_lifecycle/include/service_lifecycle.h` and `components/service_lifecycle/src/service_lifecycle.c` (+ `CMakeLists.txt`) — final names/paths chosen in implementation within this intent
- Service headers/sources as needed for thin adapters/wrappers or capability reporting (e.g. `components/wifi/`, `components/http_server/`, `components/websocket/`, `components/network/`, `components/berry_lang/`, `components/text_console/`, `components/shell/`, `components/bluetooth/`, `components/time_sync/`, `components/system/`, `components/filesystem/`, etc.)
- `test/` host tests for lifecycle ordering/dependency logic (if feasible)
- `docs/` updates and cross-links: this item, [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md), and references from/to related ready/needs-clarification items as applicable

## Dependencies / prerequisites
- Aligns with and enables/refines: `boot-profiles-feature-modes` (ready, boot-time gating; may adopt descriptors), `wifi-managed-start-stop` (ready), and needs-clarification items for HTTP/WebSocket, network diagnostics, console/shell lifecycle, observability.
- Per-service deep lifecycle behavior may remain partially specified in those items; this spec defines the shared contract they should align to (adapter-ready).
- Implementation should not block on unresolved details of other needs-clarification items beyond the shared contract defined here.

## Implementation order / prerequisites (resolved, 2026-11-16 review)
This plan provides the **shared lifecycle contract/base** (static descriptors, states/capabilities, dependency/order/fail-fast/best-effort-stop semantics, boot-run helpers). Ordered gate (authoritative):

1. **Lifecycle contract lands first:** `unified-service-lifecycle` descriptors/types/states/capabilities/boot helpers must **land** before `boot-profiles-feature-modes` implements profile/service-gating and before `optional-http-websocket-service` is implemented. Do not ship a parallel permanent or temporary ad-hoc boot/service-gating model in `main.c`.
2. **May proceed adapter-ready in parallel (now):** `wifi-managed-start-stop` may proceed as Wi-Fi-specific lifecycle/API/persistence work, but must expose hooks/capabilities/state compatible with this contract (no divergent lifecycle states) so it can be represented as a descriptor/adapter without rework.
3. **Blocked until (1) has landed:** `boot-profiles-feature-modes` must implement boot-time gating on descriptor-driven boot/control using unified-lifecycle interfaces/hooks (no temporary migration path). `optional-http-websocket-service` is blocked until required descriptors/capabilities/integration points for HTTP server/WebSocket have landed (service-specific deep teardown remains owned there except minimal contract adaptation).
4. **Consumers must stay compatible:** `service-memory-budget-observability`, `berry-runtime-service-inspection-v1`, `text-console-memory-lifecycle`, `shell-input-service-lifecycle`, and other dependent items must consume/report these states/capabilities once available and remain correct with `unsupported`/`n/a`/documented limitations where controls or metrics are not yet implemented.

Delivery slices (framework+contract+helpers → boot wiring parity → remaining adapters/integration) remain as defined above; each slice passes scope-appropriate validation before dependent work relies on it.

## Risks / constraints
- Teardown ordering and in-flight operations (tasks, sockets, callbacks, console ownership, WebSocket clients) can cause races or crashes; v1 is best-effort and primarily boot/internal serialized.
- ESP-IDF subsystems may have partial/no safe deinit paths; some memory may not be reclaimable (must be documented per service/limitation).
- Refactoring `app_main` risks subtle boot regressions; preserve existing conditional wiring and logs.
- Resource-constrained ESP32: lifecycle machinery must remain minimal (static data preferred).
- Service-specific items may require follow-up adapter changes to fully conform to the contract.

## Open questions
- None. (Resolved 2026-11-16: static descriptors; framework + boot integration v1 with core boot set in new shared component; boot/internal control with compatible wrappers, no new general runtime user commands in v1; explicit deps with fail-fast start and best-effort reverse-order stop, non-reclaimable memory documented; validation = automated AGENTS checks + feasible host tests + device smoke boot, deeper reclaim evidence deferred to dependent items; approval to be recorded explicitly after spec.)

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus).
- 2026-11-16 (clarify-feature, agent): loaded `clarify-feature` skill; reviewed `docs/README.md`, this item, `docs/plan/consolidated-improvement-plan.md` (2026-11-16 audit follow-ups), `main/main.c` boot sequence, service headers/stop APIs, and related ready/needs-clarification items (`boot-profiles-feature-modes`, `wifi-managed-start-stop`, `optional-http-websocket-service`).
- 2026-11-16 (clarify-feature, user, round 1): selected **Static descriptors** for v1 design/scope; **Boot/internal + compat wrappers** control/API compatibility; **Explicit deps, best-effort stop** semantics; **Approve + mark ready now** once spec complete (pending explicit final approval).
- 2026-11-16 (clarify-feature, user, round 2): selected **Framework + boot integration** boundary (per-service deep teardown owned by specific items except minimal contract/adapter work); **Core boot set + new component** (all `app_main` services as descriptors/adapters, core vs manageable documented, init-only where stop unsupported); **Automated + boot behavior checks** validation (AGENTS checks + feasible host tests + device smoke boot; deeper reclaim evidence deferred to dependent items).
- 2026-11-16 (clarify-feature, agent): drafted complete spec in-place (scope/non-goals/contract/acceptance/validation/files/deps/risks), set open questions to none.
- 2026-11-16 (clarify-feature, user): explicitly approved spec and pre-approved implementation/PR flow; marked Ready and moved to `docs/plan/ready/unified-service-lifecycle.md`. No implementation or validation run in this clarification step.
- 2026-11-16 (review follow-up 2): collapsed duplicated/contradictory implementation-order bullets into one authoritative sequencing gate (lifecycle first; wifi adapter-ready in parallel; boot-profiles/http-ws blocked until landed; consumers compatible). No implementation/validation yet.
- 2026-11-16 (implementation, `feat/unified-service-lifecycle`): implemented shared static-descriptor lifecycle component (`components/service_lifecycle/include/service_lifecycle.h`, `components/service_lifecycle/src/service_lifecycle.c`, `components/service_lifecycle/CMakeLists.txt`) with states/capabilities, dependency fail-fast boot, and best-effort reverse-order stop with unsupported-stop reporting.
- 2026-11-16 (implementation): refactored `main/main.c` to descriptor-driven boot/init for the core boot set (core/always-on: display, calibration, light sensor, RGB LED, brightness, text console, I2C bus, filesystem, system, shell; manageable/optional: wifi, network, bluetooth, time_sync, http_server, websocket with `websocket -> http_server` dependency), preserving default behavior/logging and conditional wiring (keyboard routing, console output broadcast, websocket text input).
- 2026-11-16 (implementation): existing public init/stop/deinit APIs left unchanged (compatible wrappers/adapters remain available); services without stop hooks/capability are represented as init/start-only and reported as unsupported-stop by framework stop handling.
- 2026-11-16 (validation): `make format-check` — pass (after `make format`). `make test` — pass in this environment (CTest: 26/26 passed, including new `test_service_lifecycle`; Python `scripts` unittest: 3/3 OK). `make docs-check` — pass. `make build` / `make lint` not run (ESP-IDF v6.1 / clang-tidy environment not verified in this session); **blockers/unverified:** firmware build+lint and on-device smoke boot/default-behavior validation remain required when environment/hardware available (deep stop/reclaim/memory evidence deferred per plan). No hardware validation performed.
- 2026-11-16 (limitations, documented): Wi-Fi/HTTP/WebSocket/network diagnostics/Bluetooth/time-sync/text-console/shell deep teardown and resource reclamation remain owned by dependent items; ESP-IDF/shared-subsystem memory may be non-reclaimable and is not assumed freed by this framework. Boot/internal serialized v1 control only; no new general runtime user service-control command added.
