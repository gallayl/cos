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
- Support **repeated `bt off` / `bt on`** (including scan/connect flows) within one boot session, with **best-effort reclamation** of BT-component-owned resources on disable. Re-enable must remain usable.
- Preserve existing external semantics: `bluetooth_init/enable/disable/scan/connect/disconnect/forget/is_*` API behavior, `bt` command availability/contract where practical, and NVS auto-enable-on-boot behavior. Changes are internal allocation/lifecycle mechanics only.
- Document unavoidable/unreclaimable memory (including ESP-IDF/controller/Bluedroid allocations that cannot be safely freed) and any safe teardown boundaries.
- **BT-local scope:** do not implement a global service manager/unified service lifecycle in this item; keep design compatible for later alignment with `unified-service-lifecycle`.

## Non-goals
- New Bluetooth profiles/features, pairing UX changes, or scan/connect behavior changes unrelated to lifecycle/memory.
- Enabling the full BT stack at boot.
- Global service registry/manager or cross-component lifecycle refactor (tracked separately by `unified-service-lifecycle`).
- Deep BLE coexistence/performance tuning except lifecycle implications.
- Forcing full reclamation of ESP-IDF BT stack memory if unsafe/unsupported on ESP32.

## Acceptance criteria
- [ ] While Bluetooth is disabled (including after boot when not auto-enabled, and after `bt off`), the BT component does not keep the scan-results array resident and does not keep unnecessary BT-only scan/close synchronization allocations beyond the agreed minimal/lightweight state required for compatible init/command/NVS behavior.
- [ ] `bluetooth_init()` remains compatible with current boot usage and external semantics: `bt` command is available as today, NVS auto-enable behavior is preserved, and existing init callers do not require behavior changes.
- [ ] Repeated `bt off` / `bt on` cycles in one boot session work correctly; scan/connect/disconnect/forget flows remain correct across cycles; disable releases BT-component-owned scan buffer and synchronization resources that are safe to release.
- [ ] Scan behavior remains compatible (same result capacity/fields where practical, HID-device scan/connect/auto-connect flows continue to work, including addr/type handling needed by current connect paths).
- [ ] Public API and `bt` command semantics remain compatible (no intentional contract redesign in this item).
- [ ] Unavoidable/unreclaimable memory and ESP32 BT teardown constraints are explicitly documented (code/docs as appropriate for implementation).
- [ ] Automated checks required by `AGENTS.md` for affected scope pass (at minimum firmware formatting/tests/build/lint as applicable for touched C/C++/build scope).

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
