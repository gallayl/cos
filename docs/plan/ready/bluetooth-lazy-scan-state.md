# Bluetooth lazy scan state and lifecycle memory

- **ID:** bluetooth-lazy-scan-state
- **Status:** ready (approved)
- **Created:** 2026-11-16
- **Source/links:** Repository audit requested 2026-11-16 (focus: unified service infrastructure, memory consumption). Findings: `main/main.c:136`, `components/bluetooth/src/bluetooth.c:38-52`, `components/bluetooth/src/bluetooth.c:483-501`, `components/bluetooth/src/bluetooth.c:549-591`. Cross-cutting tracker: [`docs/plan/consolidated-improvement-plan.md`](../consolidated-improvement-plan.md#2026-11-16-audit-follow-ups-needs-clarification). Related: [`docs/plan/ready/unified-service-lifecycle.md`](./unified-service-lifecycle.md) (separate ready item; alignment/follow-up).
- **Approval:** approved by user in chat on 2026-11-16 after specification clarification; **pre-approved for implementation/PR flow** (merge remains human).

## Summary
Bluetooth initialization runs at boot and currently keeps resident scan-related state/synchronization objects even before the BT controller/Bluedroid/HID stack is enabled. Since Bluetooth is optional for many use cases, scan/results storage and related BT-component synchronization resources should be allocated lazily and released on disable where safe, while preserving existing external API/command/boot behavior.

## Scope
- Keep `bluetooth_init()` as the boot-time entry point used by `main/main.c`: register the `bt` shell command and retain lightweight state/NVS auto-enable behavior compatible with current semantics.
- Minimize resident memory while Bluetooth is disabled: do not keep the scan-results buffer or unnecessary BT-only scan/close synchronization objects allocated at init; allocate them on first enable/scan (or other agreed safe point required by current flows) and free/reuse according to the clarified lifecycle.
- Use a **lazy fixed-capacity scan buffer** with the same 20-entry limit/fields as today (`BT_SCAN_MAX_RESULTS`, current scan result contents including name/metadata needed by existing flows); avoid disabled-state residency.
- Support **repeated `bt off` / `bt on`** (including scan/connect flows) within one boot session, with **best-effort reclamation** of BT-component-owned resources on disable. Re-enable must remain usable when Bluetooth is allowed by the active boot profile.
- Preserve existing external semantics: `bluetooth_init/enable/disable/scan/connect/disconnect/forget/is_*` API behavior, `bt` command availability/contract where practical, and stored NVS auto-enable value/behavior. **Boot-profile precedence:** in profiles that allow Bluetooth (`default`), stored NVS auto-enable applies as today; in profiles that force Bluetooth off (`low-memory-offline` per `boot-profiles-feature-modes.md`), `bluetooth_init()` still runs but auto-enable/enable is suppressed for that boot, the stored NVS auto-enable value is preserved (not cleared), `bt` remains available for status/recovery visibility, and enable/scan/connect attempts must fail clearly as disabled-by-profile without starting the BT stack. Changes are internal allocation/lifecycle mechanics only, except this explicit profile-gating behavior.
- Document unavoidable/unreclaimable memory (including ESP-IDF/controller/Bluedroid allocations that cannot be safely freed) and any safe teardown boundaries.
- **BT-local scope:** do not implement a global service manager/unified service lifecycle in this item; keep design compatible for later alignment with `unified-service-lifecycle`.

## Non-goals
- New Bluetooth profiles/features, pairing UX changes, or scan/connect behavior changes unrelated to lifecycle/memory.
- Enabling the full BT stack at boot.
- Global service registry/manager or cross-component lifecycle refactor (tracked separately by `unified-service-lifecycle`).
- Deep BLE coexistence/performance tuning except lifecycle implications.
- Forcing full reclamation of ESP-IDF BT stack memory if unsafe/unsupported on ESP32.

## Acceptance criteria
- [ ] While Bluetooth is disabled (including after boot when not auto-enabled, after `bt off`, and when forced off by boot profile), the BT component does not keep the scan-results array resident and does not keep unnecessary BT-only scan/close synchronization allocations beyond the agreed minimal/lightweight state required for compatible init/command/NVS behavior.
- [ ] `bluetooth_init()` remains compatible with current boot usage and external semantics: `bt` command is available as today in all profiles that call init (including `low-memory-offline`), stored NVS auto-enable value is preserved, and NVS auto-enable behavior is applied only when the active boot profile allows Bluetooth (forced-off profiles suppress enable/auto-enable for that boot with clear disabled-by-profile behavior and no stored-value mutation).
- [ ] Repeated `bt off` / `bt on` cycles in one boot session work correctly when Bluetooth is allowed by profile; scan/connect/disconnect/forget flows remain correct across cycles; disable releases BT-component-owned scan buffer and synchronization resources that are safe to release.
- [ ] Scan behavior remains compatible (same result capacity/fields where practical, HID-device scan/connect/auto-connect flows continue to work, including addr/type handling needed by current connect paths).
- [ ] Public API and `bt` command semantics remain compatible (no intentional contract redesign in this item), with explicit profile-forced-off errors where applicable.
- [ ] Unavoidable/unreclaimable memory and ESP32 BT teardown constraints are explicitly documented (code/docs as appropriate for implementation).
- [ ] Automated checks required by `AGENTS.md` for affected scope pass (at minimum firmware formatting/tests/build/lint as applicable for touched C/C++/build scope). Hardware/manual validation required when available for this item must be recorded or concretely blocked per `AGENTS.md`.

## Validation plan
- **Automated (required for affected firmware scope):** `make format-check`, `make test`. For firmware/build/lint-covered changes: `make build` and `make lint` when feasible in an ESP-IDF v6.1 environment; report exact commands/outcomes and any environment blockers per `AGENTS.md`. `make docs-check` if documentation/Doxygen scope is affected.
- **Hardware/manual (required):** on ESP32/CYD device validate (1) boot with BT disabled in NVS: `bt` available, no regressions; (2) boot with BT auto-enable in NVS: auto-enable/connect behavior preserved; (3) `bt on`/`scan`/`connect`/`disconnect`/`forget`/`bt off` happy paths and expected errors; (4) repeated `bt off`/`bt on` cycles in one boot session; (5) observe/report heap/free/largest-block/min-free (or equivalent) while disabled vs enabled/between cycles to evidence reduced disabled-state residency (exact measurement method/commands to be captured during implementation/validation; reuse `service-memory-budget-observability` `svc_mem`/A-B checklist where useful). Wi-Fi/network coexistence sanity check if BT is exercised alongside networking.

## Expected areas/files
- `components/bluetooth/src/bluetooth.c` (lazy allocation/free, lifecycle/state handling)
- `components/bluetooth/include/bluetooth.h` (only if docs/API comments or signatures need clarification; semantics preserved)
- `components/bluetooth/src/bluetooth_cmd.c` (only if needed to preserve command behavior/error handling)
- `components/bluetooth/src/bluetooth_hid.c` / `bluetooth_hid.h` (only if lifecycle interactions require changes)
- `main/main.c` (verify init sequencing/boot behavior remains compatible; changes only if necessary)
- `docs/` updates to document reclaimable vs unavoidable memory/teardown constraints as applicable

## Dependencies / prerequisites
- **`unified-service-lifecycle` (`docs/plan/ready/unified-service-lifecycle.md`):** alignment/follow-up only; this item proceeds as BT-local work and must remain compatible with future unified lifecycle, but does not depend on that item being clarified/implemented first.
- ESP32/ESP-IDF BT stack capabilities/constraints for deinit, repeated enable/disable, and memory reclamation.
- Wi-Fi/network coexistence constraints on ESP32 when BT is enabled (validation consideration).

## Risks / constraints
- BT stack init/deinit on ESP32 can be fragile; aggressive teardown may be unsupported or destabilizing. Only BT-component-owned resources that are safe to release should be freed.
- Some BT/controller/Bluedroid memory may not be reclaimable after enable/disable; this must be documented rather than assumed.
- Current flows assume scan results/sync objects exist at certain points (e.g. scan, auto-connect address/type lookup/injection, close synchronization). Lazy allocation introduces allocation-failure paths and ordering requirements that must be handled cleanly.
- Repeated enable/disable increases risk of races/leaks/dangling callbacks/tasks; state transitions and synchronization must remain correct.
- Resource-constrained target: implementation must stay small and avoid new steady-state overhead.

## Decisions (2026-11-16 clarification)
- **Boot/init responsibilities:** lazy-allocate scan buffer and scan/close synchronization resources (not resident from init for disabled state), while keeping `bluetooth_init()` compatible (registers `bt`, lightweight state, preserves NVS auto-enable behavior).
- **Repeated cycles:** support repeated `bt off` / `bt on` in one boot session (including scan/connect), with best-effort reclamation of safe BT-component-owned resources on disable; document non-reclaimable stack memory.
- **Scan storage:** lazy fixed-capacity buffer, same 20-entry limit/fields as current; free on disable (or equivalent safe lifecycle point) so disabled state avoids resident scan array.
- **Compatibility:** preserve existing external API semantics, `bt` command contract/availability where practical, and NVS auto-enable-on-boot behavior.
- **Scope vs unified lifecycle:** BT-local lifecycle/memory optimization now, designed to align with future `unified-service-lifecycle` (no global service manager in this item).
- **Memory/complexity target:** reclaim disabled-state BT scan/sync RAM with modest complexity; document estimated/measured savings and unavoidable allocations (no specific byte target required).
- **Approval intent:** user will explicitly approve moving to `docs/plan/ready/` in a follow-up after the specification is complete and has no open questions.

## Open questions
- None.

## Progress / evidence
- 2026-11-16: created from repository audit (unified service infrastructure/memory focus).
- 2026-11-16: clarification round completed in chat (question tool). Recorded decisions: lazy init-compatible allocation of scan/sync resources, repeated enable/disable with best-effort reclaim, lazy fixed-capacity scan buffer (20 entries) freed on disable, preserve external API/`bt`/NVS auto-enable semantics, BT-local scope aligned for future unified lifecycle, modest-complexity disabled-state RAM reclamation target. Approval deferred pending follow-up.
- 2026-11-16: user explicitly approved in chat after complete specification; item marked ready and moved to `docs/plan/ready/bluetooth-lazy-scan-state.md` (pre-approved for implementation/PR flow).
- 2026-11-16 (review follow-up): aligned with `boot-profiles-feature-modes` — `bluetooth_init()` runs in all profiles; `low-memory-offline` forces BT off for the boot while preserving stored NVS auto-enable and `bt` command compatibility with clear disabled-by-profile behavior. Hardware validation wording clarified to match AGENTS.
- 2026-11-16 (implementation, `feat/bluetooth-lazy-scan-state`): implemented BT-local lazy runtime resources in `components/bluetooth/src/bluetooth.c`. `bluetooth_init()` now only registers `bt` commands and preserves NVS auto-enable behavior (no resident scan buffer or scan/close semaphores at init). Added heap-allocated lazy scan buffer (`BT_SCAN_MAX_RESULTS` entries, same fields) and lazily allocated `s_scan_done`/`s_scan_params_set`/`s_close_done`, ensured before enable/start and on runtime paths as needed; resources are released on successful `bluetooth_disable()` and on enable failure (best-effort reclamation of BT-component-owned scan/sync allocations). `bluetooth_scan`/`bluetooth_forget`/lookup paths guarded for lazy state; scan/connect/disconnect/forget/NVS auto-enable flows kept compatible. No changes to public API signatures or `bt` command contract.
- 2026-11-16 (local validation): `make format` + `make format-check` — pass. `make test` — pass (CTest 26/26, Python `unittest discover -s scripts -p "test_*.py"` 3/3 OK). `make build` — pass in local ESP-IDF 6.1 environment (`cos.bin` generated, reported `0x1e4c10` bytes in 0x200000 app partition, 5% free). `make lint` — **failed** in current environment with pre-existing/unrelated issues including `main/main.c` clang-diagnostic error `service_lifecycle.h` file not found and numerous suppressed warnings outside this change; no bluetooth-specific lint errors identified in captured tail output. Hardware/manual validation on ESP32/CYD (boot disabled/enabled NVS cases, scan/connect/disconnect/forget, repeated `bt off`/`bt on` cycles, heap evidence, coexistence sanity) **not run** in this environment — required per plan when hardware/environment is available. Doc state kept in `ready/` until delivered/PR CI green and finalized per workflow.
- 2026-11-16 (review fix, independent `reviewer` subagent): addressed actionable findings (High/Medium) in `components/bluetooth/src/bluetooth.c` — made `bt_ensure_runtime_resources()` transactional/all-or-nothing (releases partial allocations/semaphores on failure) and released runtime resources on `bluetooth_enable()` task-creation failure; prevents lazy scan/sync resource retention on synchronous enable-path failures (including auto-start attempts) while disabled. Re-ran `make format-check` and `make test` after fix (see follow-up commit/PR updates). Hardware/manual validation still pending.
- 2026-11-16 (hardware test follow-up, user-reported): `bt on` failed with `ESP_HIDH: esp_hidh_init(108): Already initialized` / `ESP_ERR_INVALID_STATE` on re-enable in device session. Added defensive HID host reinit handling in `do_bt_enable_sequence()` (on `ESP_ERR_INVALID_STATE`, deinit and retry once) to recover from stale HID host state. Re-ran `make format-check` and `make test` (26/26, 3/3). On-device revalidation of off/on cycles still required.
