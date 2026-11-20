# Pending hardware validation inventory and console-session preparation (2026-11-18)

- **Date:** 2026-11-18
- **Branch/work context:** `docs/hw-validation-pending-items` (docs/prep only in this session)
- **Scope:** inventory of features/items that still require hardware/manual validation, with console-automatable command plans prepared for `scripts/hw_console_session.py` (no `/flash` writes). **No on-device validation was executed in this session** (serial port/device state not provided; `pyserial` not installed in active Python env).
- **Rule applied:** do **not** write to `/flash` (flaky on device). Command plans avoid file writes to `/flash` and avoid `make flash-data`/erase/flash operations.

## Reusable templates (recommended, docs-only)

- [HW validation preflight checklist](./templates/hw-validation-preflight.md) — capture port/firmware/profile/storage/reboot/environment/tooling/blockers and a quick read-only probe/command plan before device sessions.
- [HW validation session & evidence](./templates/hw-validation-session.md) — record batched session metadata, per-item outcomes with evidence paths/logs (`pass`/`fail`/`inconclusive`/`waived`), anomalies, and explicit waiver fields (rationale, approver, context, date) when needed. Batched sessions do not replace per-item traceability.
- [PR/plan delivery evidence & finalization checklist](./templates/delivery-evidence-checklist.md) — record exact local checks, CI evidence, HW/manual status per item, pending human actions, and queue/cross-link/finalization steps.

These templates are **recommended practice** (not CI-enforced). Use them to improve traceability for HW validation passes and merge-ready/done or HW-closure claims without weakening `AGENTS.md`, `docs/README.md`, or required gates (merge/review/approval and hardware validation remain human actions unless explicitly waived with rationale recorded). See [`docs/README.md`](../README.md).

## Baseline / local checks executed in this session (evidence)

| Command | Result | Notes |
|---|---|---|
| `make format-check` | **pass** | exit 0 |
| `make test` | **pass** | CTest **30/30 passed**, Python `scripts` unittest **3/3 OK** (exit 0) |
| `make docs-check` | **pass** | `doxygen` completed (exit 0) |

**Not run:** `make build`, `make lint` (ESP-IDF v6.1 / clang-tidy environment not verified in this session), frontend checks (no frontend changes), integration/QEMU suites (not required for this docs-only prep).

## Preconditions to execute on-device validation (per `on-device-validation` skill)

Provide before running sessions:
- **Serial port and baud** (default 115200), e.g. `COMx`.
- **Device state:** firmware already flashed, **commit/build/ELF identity** of running image, boot profile, storage state, whether reboot is acceptable.
- **Environment:** Wi-Fi credentials/network availability, SD card presence, Bluetooth peripherals/lighting/touch conditions as needed.
- **Tooling:** Python environment with `pyserial` for `scripts/hw_console_session.py`.

### Automation blocker / remediation (current environment)
- `python --version`: `Python 3.14.7`
- `python -c "import serial; print(serial.__version__)"`: **ModuleNotFoundError: No module named 'serial'**
- **Fix:** install in the active environment (`python -m pip install pyserial`) or run sessions from an ESP-IDF Python environment that includes `pyserial` (`%IDF_PYTHON_ENV_PATH%`), or capture manually via `idf.py -p <PORT> monitor` and save transcript as evidence.

## Known issues/blockers affecting validation (from existing evidence)

- **Critical (recorded 2026-11-17, firmware ELF SHA256 `6b9787961` in captures):** in `low-memory-offline` profile, `services` and `svc_mem` can assert/panic (`xQueueSemaphoreTake queue.c:1709 ((pxQueue))`) via `websocket_client_count` → `service_diagnostics` collection when WebSocket is unavailable/not initialized, causing reboot. This blocks safe console validation of `services`/`svc_mem` and parts of boot-profile/inspection/observability validation in that profile until fixed and reflashed. Evidence: `docs/reports/2026-11-17-hardware-validation-session-captures.md` (R6/R8 follow-up context).
- **R20 pipes follow-up (2026-11-17 capture):** `berry eval "print('x')" | mem` executed `mem` instead of a handled Berry-only-`rhs` unsupported-consumer diagnostic (needs investigation/retest on current firmware).
- **General:** many items require physical/manual actions (touch gestures, lighting changes, BT pairing/input, UI app interaction, reboot/profile changes, visual confirmation). Console automation can capture only a subset.

## Inventory (pending hardware/manual validation)

| Item | Location / status | What remains | Console-automatable (no `/flash` writes) | Manual / hardware-only | Prepared plan(s) |
|---|---|---|---|---|---|
| R12 Milestone 4 hardware validation | `docs/plan/needs-clarification/r12-milestone4-hardware-validation.md` (needs-clarification), tracker R12 pending | Touch scrollback gestures; connected-network `ping`/`nslookup` under current acceptance interpretation; recorded evidence/docs updates | `wifi status`, `services`, `ping 8.8.8.8`, `ping 127.0.0.1`, `nslookup example.com` (if network available) | Touch scrollback gestures, screenshots/behavior notes, acceptance clarification | `docs/reports/artifacts/hw-pending-2026-11-18/plans/r12-network.txt` |
| R22 M3 `light` | `docs/plan/needs-clarification/r22-m3-light-hardware-validation.md` (needs-clarification) | Documented lighting sweep (0–4095 behavior across conditions), explicit path/API sanity in controlled captures, error/unavailable path or N/A with evidence/rationale | `light`, `/flash/scripts/light.be`, `berry eval "var m = cos_import('@cos/light'); var l = m.find('default'); print(l.read())"` | Physical lighting changes, sensor-failure reproduction may be N/A | `docs/reports/artifacts/hw-pending-2026-11-18/plans/r22-m3-light.txt` |
| Unified service lifecycle smoke boot/default behavior | `docs/plan/done/unified-service-lifecycle.md` | On-device smoke boot/default startup behavior and service availability equivalent to pre-lifecycle behavior | Boot/startup logs capture (with reset if safe), `services`, `boot_profile`, `wifi status`, `mem`, `berry profile` after boot | Visual/boot observation, default-profile environment confirmation | `docs/reports/artifacts/hw-pending-2026-11-18/plans/unified-lifecycle-smoke.txt` |
| Optional network diagnostics worker | `docs/plan/done/optional-network-diagnostics-worker.md` | `ping`/`resolve` correctness, timeout/abandon and concurrent/in-flight cases, lazy-create failure/retry behavior, heap/task/resource observation (no boot-resident diagnostics resources, no leaks), `low-memory-offline` disabled-by-profile/no-lazy-create path | `services`, `berry profile`/`mem`, `ping`/`nslookup`/`resolve` as available, repeated calls; low-memory checks only if safe (see crash blocker) | Wi-Fi disconnected/timeout/abandon scenarios, profile reboots, resource observation beyond cheap counters, low-memory profile safety depends on fix | `docs/reports/artifacts/hw-pending-2026-11-18/plans/network-diagnostics.txt` |
| Wi-Fi managed start/stop (detailed closure) | `docs/plan/done/wifi-managed-start-stop.md` | Any remaining formal evidence per validation plan not fully recorded in-repo (persisted-disabled reboot, profile-forced-off case, repeated cycles/stability, memory/power observations as feasible) | `wifi status/start/stop/restart/scan/connect/disconnect` sequences, `services`, `mem`/`svc_mem` snapshots (avoid low-memory crash paths) | Reboots, profile-forced-off reboot behavior, environment-dependent connect/scan, repeated cycle stress, power observations | `docs/reports/artifacts/hw-pending-2026-11-18/plans/wifi-lifecycle.txt` |
| Text console memory & lifecycle | `docs/plan/done/text-console-memory-lifecycle.md` | Heap/watermark/task/stack impact across transitions; hide/show and app/display-ownership transitions (scrollback auto-release, not preserved on return, base buffer/output capture on return); UART/log/listener correctness while hidden; rendering/touch suspended/restored; no regressions | Baseline `mem`/`berry profile`; limited console-visible checks; `berry run`/UI app launch+close if app exists on device storage (read-only) | Physical visibility/app-ownership transitions, touch/visual rendering, listener/UART behavior while hidden, heap/watermark evidence across transitions | `docs/reports/artifacts/hw-pending-2026-11-18/plans/text-console.txt` |
| Boot profiles & feature modes | `docs/plan/done/boot-profiles-feature-modes.md` | Per-profile reboot/service availability/unavailability, recovery path, memory evidence (A-B) | `boot_profile` get/set/clear/status output, `services`, `bt` behavior, `mem`/`berry profile`; **avoid `services`/`svc_mem` in `low-memory-offline` on affected builds** | Reboots between profiles, serial/on-device recovery validation, full availability matrix confirmation | `docs/reports/artifacts/hw-pending-2026-11-18/plans/boot-profiles.txt` |
| R20 pipes runtime v1 (CYD smoke) | `docs/plan/done/r20-pipes-runtime-v1.md` | Required CYD smoke: happy path Berry producer `|` Berry consumer using `console.stdin()` with expected output/status and console restored; one handled failure/diagnostic case; investigate/retest `non-Berry rhs | mem` behavior | Pipe commands if `pipe_stdin.be` exists on mounted storage (read-only): `berry eval "print('PIPE_HELLO')" \| /flash/scripts/pipe_stdin.be` (or actual path), 3-stage/pipe+redirect/overflow diagnostics, `berry eval "print('x')" \| mem` | Visual confirmation of stdin/log behavior, console restoration checks, device/storage path notes | `docs/reports/artifacts/hw-pending-2026-11-18/plans/r20-pipes.txt` |
| Service memory budget observability (`svc_mem`) | `docs/plan/done/service-memory-budget-observability.md`, `docs/reports/2026-11-17-svc-mem-memory-budget-observability.md` | On-device `svc_mem` captures/sign-off, repeated-read stability, controllable start/stop before/after, R2-style A-B for services without runtime stop, device-measured reference ranges/watermarks | Baseline/repeated `svc_mem`, `mem`, `berry profile`, Wi-Fi A-B snapshots where safe | Long-run/workload observations, low-memory profile captures blocked by crash on affected builds, hardware sign-off | `docs/reports/artifacts/hw-pending-2026-11-18/plans/svc-mem.txt` |
| Berry runtime & service inspection v1 | `docs/plan/done/berry-runtime-service-inspection-v1.md` | Full on-device state coverage for `services`/`berry app`/`berry profile`/`mem`, varied service states, low-memory-offline safety (currently crashes on affected builds) | `services`, `berry app`, `berry profile`, `mem`; active-app check if UI app available on storage (`berry run ...`, `berry app`, `berry close`) | UI app interaction/visibility, profile reboot cases, crash fix validation if firmware updated | `docs/reports/artifacts/hw-pending-2026-11-18/plans/berry-inspection.txt` |
| Bluetooth lazy scan state (residual gaps) | `docs/plan/done/bluetooth-lazy-scan-state.md` | Heap/free/largest/min observations while disabled vs enabled/between cycles, NVS auto-enable boot case, `bt forget` flow, Wi-Fi/network coexistence sanity during BT exercise | `bt`/`bt status`/`bt on`/`bt off`/`bt scan` (if safe), `mem`/`svc_mem` snapshots around cycles | Pairing/connect/input with BT peripheral, `bt forget`, NVS auto-enable reboot case, coexistence sanity, physical environment | `docs/reports/artifacts/hw-pending-2026-11-18/plans/bluetooth.txt` |

## Recommended execution order (console sessions, when device available)

1. **Read-only baseline (default profile, no reboot):** `hw-baseline-readonly.txt` — capture identity hints (`boot_profile`, `services`, `wifi status`, `mem`, `berry profile`, `svc_mem`, `uptime`, `info`, `path list`) and quick checks for R12 network, R22 M3 light, R22 M4 time, R20 diagnostics subset, inspection idle state. Avoid state-changing commands except explicitly planned.
2. **Network/Wi-Fi lifecycle session:** run targeted commands from Wi-Fi/network plans; record reconnect/scan/ping/resolve behavior and A-B memory snapshots. Requires network credentials/environment.
3. **Berry app/pipes session:** only if required apps/scripts exist on mounted storage (`/sdcard` or existing `/flash` scripts/apps) and no writes are needed.
4. **Bluetooth session:** requires BT peripheral/environment and careful handling; capture memory snapshots around cycles.
5. **Profile/reboot sessions (manual-heavy):** `boot_profile set ...`, reboot, capture evidence, then restore default. **Do not run `services`/`svc_mem` in `low-memory-offline` on firmware builds affected by the websocket diagnostics crash**; capture `boot_profile`, `bt`, `mem`, `berry profile` only until fixed.
6. **Manual-only validations:** touch scrollback (R12), text-console visibility/touch/scrollback behavior transitions, lighting sweeps, visual confirmations.

## Command plans (prepared; not executed)

- `docs/reports/artifacts/hw-pending-2026-11-18/plans/hw-baseline-readonly.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/r12-network.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/r22-m3-light.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/unified-lifecycle-smoke.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/network-diagnostics.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/wifi-lifecycle.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/text-console.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/boot-profiles.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/r20-pipes.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/svc-mem.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/berry-inspection.txt`
- `docs/reports/artifacts/hw-pending-2026-11-18/plans/bluetooth.txt`

Example (when port/pyserial/device ready, from repo root):
```powershell
python scripts/hw_console_session.py --port COM3 --baud 115200 --command-file docs/reports/artifacts/hw-pending-2026-11-18/plans/hw-baseline-readonly.txt --out docs/reports/artifacts/hw-pending-2026-11-18/logs/hw-baseline-readonly.log --timeout 20 --idle-ms 600
```

## Remaining actions (human/device required)

1. Provide serial port/baud, confirm flashed firmware commit/build, environment, and whether reboot/profile changes are acceptable.
2. Install `pyserial` in the active Python env (or use ESP-IDF Python env) for automated capture, or run `idf.py -p <PORT> monitor` manually and save transcripts.
3. Execute console sessions, perform manual/hardware-only steps, and save logs under `docs/reports/artifacts/hw-pending-2026-11-18/logs/`.
4. Update this report and relevant plan docs (`needs-clarification`, `done`, consolidated tracker) with pass/fail/inconclusive outcomes and evidence; move/close items only when criteria are satisfied per `docs/README.md` and root `AGENTS.md`.
5. If low-memory-offline `services`/`svc_mem` crash is still present on tested firmware, file/fix the diagnostics/WebSocket uninitialized-state issue and reflash before completing affected validations.
6. Review/approval and merge remain human actions.

## Addendum (2026-11-19): executed subset on COM3 (`4ef50c0`)

- Executed on-device validation (no `/flash` writes, `/sdcard` used where needed) for requested scopes **unified-lifecycle-smoke + logging + baseline read-only (adapted)**: `docs/reports/2026-11-19-hw-validation-only-missing-com3.md` and raw artifacts `docs/reports/artifacts/hw-2026-11-19-only-missing-com3/{plans,logs}/`.
- **Outcomes:** unified-lifecycle smoke console evidence captured (reset/boot + post-boot service snapshot, default profile). Logging console smoke captured (in-session `error|warn|info|debug` transitions and reboot persistence of `info` restored to `warn`; representative log-visibility across levels not separately controlled/captured). Baseline read-only adapted captured in post-recovery rerun with noted anomalies/failures (prior capture sessions inconclusive; rerun2 recorded ping timeout/NO_MEM and nslookup NO_MEM under observed heap pressure, and 2-stage pipe stdin-visible output not evidenced due to log level).
- Items from "only on-device validation missing" audit addressed in this run: `docs/plan/done/unified-service-lifecycle.md` (console smoke evidence recorded) and `docs/plan/done/logging-levels-and-audit.md` (level changes + reboot persistence evidenced; visibility aspect may still need human confirmation/waiver). `docs/plan/done/optional-network-diagnostics-worker.md` not executed in this run.
