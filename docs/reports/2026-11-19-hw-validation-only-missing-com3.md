# Hardware validation: "only on-device validation missing" items (COM3, 2026-11-19)

- **Date:** 2026-11-19
- **Branch:** `docs/hw-validation-only-missing-com3-2026-11-19` (docs/evidence)
- **Requested scope:** audit `docs/plan/done` and `docs/plan/ready` for items where only on-device validation is missing, validate using `on-device-validation` skill, and document findings. Executed scopes requested by user: **unified-lifecycle-smoke + logging + baseline read-only**.
- **Device/session:** serial **COM3**, **115200** baud, reset/reboot **OK** (used). User confirmed firmware flashed (commit/build identity not separately provided by user); running image reported in captured boot logs: **app version `4ef50c0`**, **ELF SHA256 `2e6cd1e60...`** (truncated in transcript), **ESP-IDF v6.1**, ESP32 chip rev v3.1. Default boot profile during runs; SD mounted; Wi-Fi STA connected in observed sessions (network identifiers/signal visible in raw transcripts).
- **Constraints applied:** **no `/flash` writes**; used **`/sdcard`** paths/scripts where needed. `/sdcard/scripts` present in path; existing on-device SD scripts used (no scripts uploaded/written in this run).
- **Automation:** `python scripts/hw_console_session.py` (`pyserial 3.5` installed in active Python env during this session). Raw transcripts and adapted plans: `docs/reports/artifacts/hw-2026-11-19-only-missing-com3/{plans,logs}/`.
- **Skill compliance:** inputs provided by user (port/baud, flashed, reboot OK, scope). No checks claimed as passed unless executed and evidenced in captured logs.

## 1) Audit: done/ready items where only on-device validation is missing

Reviewed all `docs/plan/done/*.md` and `docs/plan/ready/*.md` (workspace state at HEAD `4ef50c0`). "Only on-device validation missing" = implementation delivered and scoped automated/CI evidence recorded with no known blocking code defect stated in the item, and remaining gap is on-device/hardware/manual validation evidence.

| Item | Classification | Rationale (from item evidence) |
|---|---|---|
| `docs/plan/done/unified-service-lifecycle.md` | **Qualifies** | Delivered/merged (PR #41, `d6e30a6`, CI green). Remaining stated gap: on-device smoke boot/default-behavior validation evidence. |
| `docs/plan/done/logging-levels-and-audit.md` | **Qualifies** | Delivered/merged (PR #93, `e038512`, required CI green). Remaining: hardware/manual smoke (set levels, reboot persistence, representative visibility) or explicit waiver. |
| `docs/plan/done/optional-network-diagnostics-worker.md` | **Borderline (validation-gated)** | Implementation/CI evidence complete per item; hardware/manual validation pending sign-off and merge blocked until validated or explicitly waived (merge remains human). Remaining technical gap is on-device validation evidence. |
| `docs/plan/done/optional-http-websocket-service.md`, `docs/plan/done/text-console-memory-lifecycle.md`, `docs/plan/done/http-upload-memory-bounds.md` | Not only | Hardware/manual validation pending **and** human review/merge pending (http-upload-bounds also notes additional QEMU/fragmentation/core stop-abort stress gaps). |
| `docs/plan/done/berry-runtime-service-inspection-v1.md`, `docs/plan/done/boot-profiles-feature-modes.md`, `docs/plan/done/service-memory-budget-observability.md` | Not only | Known critical low-memory-offline diagnostics crash recorded on affected builds (`services`/`svc_mem` via WebSocket diagnostics collection); fix/retest needed, not just validation. |
| `docs/plan/done/r20-pipes-runtime-v1.md` | Not only | Required CYD smoke pending **and** R3 finding requires investigation/retest. |
| `docs/plan/done/bluetooth-lazy-scan-state.md` | Not only | Partial hardware validation with residual gaps. |
| `docs/plan/done/wifi-managed-start-stop.md`, `docs/plan/done/r22-m4-time-read-only.md`, `docs/plan/done/shell-input-service-lifecycle.md` | Not missing | Validation/evidence recorded for scoped closure. |
| `docs/plan/ready/*` | Not only | No ready item is implementation/CI-complete with only on-device validation remaining. |

Related prep: `docs/reports/2026-11-18-pending-hardware-validation-inventory.md` and prepared plans in `docs/reports/artifacts/hw-pending-2026-11-18/plans/`.

## 2) Executed sessions and outcomes

| Session | Log | Result | Evidence / notes |
|---|---|---|---|
| Probe (read-only snapshot) | `logs/probe.log` | **Pass (executed)** | Captured `boot_profile`, `path list` (`/sdcard/scripts`), `services`, `wifi status`, `berry app`, `mem`, `berry profile`, `svc_mem`, `uptime`, `info` (SD mounted), `logging` (`warn`), `time`/`utc`/`epoch`/`foo` and `system.time_info()`, `light` and `/sdcard/scripts/light.be`, and network `ping 8.8.8.8`/`ping 127.0.0.1`/`nslookup example.com` succeeded in this run. Confirmed `/sdcard/scripts/{light,pipe_stdin,time}.be` exist. **Issue:** `ls /sdcard` and `ls /sdcard/scripts` failed: `E (...) berry: /sdcard/scripts/ls.be: run(args) failed: 'string' value has no method 'charAt'` / `ls: ESP_FAIL` (SD-resolved `ls` script incompatible/broken on device). |
| Unified lifecycle smoke (reset + boot capture) | `logs/unified-lifecycle-smoke-reset.log` | **Pass (executed, console smoke)** | Boot logs captured after reset (app `4ef50c0`, ELF `2e6cd1e60...`, ESP-IDF v6.1, default profile). Post-boot commands succeeded: `boot_profile` default/effective/next, `services` showed HTTP server/WebSocket/Wi-Fi/Bluetooth/Network/Time sync states, `wifi status` started/AP+STA with STA IP, `berry app none`, `mem`, `berry profile`, `svc_mem` outputs captured; no crash observed. Provides on-device console smoke evidence for default startup/service availability. **Note:** `E (...) berry: /flash/autoexec.be: script must define __cos_entry or run(args)` observed during boot (pre-existing/device state, unrelated). |
| Logging set-info before reboot | `logs/logging-set-info-before-reboot.log` | **Pass (executed)** | `logging info` applied and confirmed. |
| Logging persistence after reboot | `logs/logging-persistence-after-reboot.log` | **Pass (executed, persistence evidenced)** | After reset, boot captured and `logging` reported **`info`** (persisted), then restored to **`warn`** and confirmed. **Anomalies:** `E (...) berry: Async bootstrap failed...` and final `berry eval` in session failed (`ESP_FAIL`); `W (...) wifi:m f null` warnings appeared after restoring warn. Logging persistence/restore behavior evidenced. |
| Logging smoke (in-session) | `logs/logging-smoke.log` | **Pass (executed)** | `logging` query and `logging error|warn|info|debug` each set with expected confirmation and query; restored to `warn`. |
| Baseline read-only adapted (initial) | `logs/baseline-readonly-adapted.log` | **Inconclusive (partial)** | Initial commands captured through `wifi status`/`berry app`; after `mem` TX no further RX captured for remaining commands (repeated "no output within idle..."), session ended status 0. Cause not confirmed from transcript (possible console sync/capture issue). |
| Health check | `logs/health-check.log` | **Inconclusive** | No RX captured for commands (status 0). |
| Baseline read-only rerun | `logs/baseline-readonly-rerun.log` | **Inconclusive** | No RX captured for commands (status 0). |
| Recovery check (reset) | `logs/recovery-check.log` | **Pass (executed)** | Device responsive after reset; `boot_profile`, `services`, `mem`, `logging` returned expected outputs. |
| Baseline read-only rerun2 (post-recovery) | `logs/baseline-readonly-rerun2.log` | **Pass (executed, with noted failures/anomalies)** | Core read-only checks captured: `boot_profile`, `path list`, `services`, `wifi status`, `berry app`, `mem`, `berry profile`, `svc_mem`, `uptime`, `info`, `logging`, `/sdcard/scripts/light.be`, light API eval, `time`/`utc`/`epoch`/`foo` fallback, `system.time_info()` eval, adapted pipe checks, and fs/path checks. **Observed issues:** `ping 8.8.8.8` -> `ESP_ERR_TIMEOUT`; `ping 127.0.0.1` -> `ESP_ERR_NO_MEM` (`create ping task failed`); `nslookup example.com` -> `ESP_ERR_NO_MEM` (Berry source buffer alloc failed under observed low free/largest heap). 3-stage pipe correctly reported `pipes: not supported yet`; 2-stage `print('x') | /sdcard/scripts/pipe_stdin.be` produced no visible "stdin:" console text (script uses `log()`/ESP_LOGI; global level `warn`), so stdin-visible output not evidenced in this capture. BT auto-connect failures to unavailable device appeared during startup window (environmental). No `/flash` writes used; `/sdcard` paths/scripts used. |

## 3) Findings for requested validation targets

- **`docs/plan/done/unified-service-lifecycle.md` (requested: unified-lifecycle-smoke):** on-device **console smoke-boot/default-behavior evidence captured** (reset boot + post-boot `boot_profile`, `services`, `wifi status`, `berry app`, `mem`, `berry profile`, `svc_mem`) on COM3, default profile, firmware `4ef50c0` (ESP-IDF v6.1). Evidence: `logs/unified-lifecycle-smoke-reset.log` and `logs/recovery-check.log`. Any additional human visual/physical confirmation beyond captured console transcript remains human judgment if required.
- **`docs/plan/done/logging-levels-and-audit.md` (requested: logging):** console smoke **evidenced** for supported level transitions and persistence: `logging` accepted/reported `error|warn|info|debug` in-session (`logs/logging-smoke.log`) and **reboot persistence evidenced** (`info` persisted across reset, restored to `warn`; `logs/logging-set-info-before-reboot.log`, `logs/logging-persistence-after-reboot.log`). Representative log visibility/appearance across levels on UART/on-screen text console was **not** separately controlled/captured beyond command responses and incidental runtime messages; treat as not fully evidenced.
- **`docs/plan/done/optional-network-diagnostics-worker.md`:** **not executed** in this run (not requested).
- **Baseline read-only (requested, adapted):** executed with evidence in `logs/baseline-readonly-rerun2.log` after inconclusive capture attempts (`logs/baseline-readonly-adapted.log`, `logs/health-check.log`, `logs/baseline-readonly-rerun.log`) and recovery reset (`logs/recovery-check.log`). Network diagnostics in rerun2 showed memory-pressure/timeout outcomes under observed heap conditions; recorded as environment/device-state observations, not general claims.

## 4) Additional observations (not item closures)

- SD-resolved `ls` script failed on device (`/sdcard/scripts/ls.be`: `'string' value has no method 'charAt'`), while `fs.exists` checks worked. Firmware/runtime and SD script set may be mismatched; not modified (no `/flash` writes, no SD script changes).
- Flash `autoexec.be` invalid for entrypoint contract (`script must define __cos_entry or run(args)`).
- Some sessions became non-capturing/inconclusive; device recovered after reset. Transcripts preserved for transparency.

## 5) Remaining actions

- **Human:** review/approve/merge PR with evidence.
- **Human/device (if required):** any additional on-screen/visual confirmation for unified-lifecycle smoke and representative log visibility for logging; validation for `optional-network-diagnostics-worker` and other pending items per their plans (see `docs/reports/2026-11-18-pending-hardware-validation-inventory.md` and `docs/reports/artifacts/hw-pending-2026-11-18/plans/`, respecting known low-memory-offline diagnostics crash caveat on affected builds).
- **Not claimed:** no unexecuted checks claimed passed; inconclusive sessions recorded as such.
