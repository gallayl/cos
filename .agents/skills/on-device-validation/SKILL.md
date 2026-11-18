---
name: on-device-validation
description: Run a generic on-device (hardware) validation session over the serial esp_console: execute a planned command/script sequence on a flashed device, capture a timestamped transcript, and report evidence with manual gaps. Use when the user provides a serial port (e.g. COM3) and wants automated console-driven hardware checks.
---

# On-device validation (generic)

Automate the serial-console part of hardware validation: send commands to a flashed device, record the full transcript, and report evidence. This reduces manual pasting in `idf.py monitor`; it does not replace checks that require physical interaction.

## Inputs (get from user before running)

- **Serial port** and baud (default `115200`), e.g. `COM3` / `/dev/ttyUSB0`.
- **Device state:** firmware already flashed and which image/build/commit if known; whether a reset/boot is acceptable.
- **Command plan:** ordered commands and/or Berry script invocations to run (inline list or a command file), plus the expected output/markers for each step when known.
- **Artifact location:** where to save logs (default under `docs/reports/artifacts/hw-<date>-<topic>/` when producing repo evidence; otherwise a path the user specifies).
- **Manual preconditions:** Wi-Fi credentials/network, SD inserted, lighting/touch/button conditions, or any step the agent cannot perform.

Do not guess the port or claim the device is flashed. If flashing is needed, the user flashes (e.g. `make flash`, optionally `make flash-data PORT=<port>`); do not erase/flash unless explicitly asked.

## Procedure

1. **Preflight.** Confirm the port is correct/available and the session will not conflict with another monitor. Automation uses a scripted serial driver (not an interactive `idf.py monitor` TUI). Prefer `scripts/hw_console_session.py`, which requires `pyserial` in the active Python environment (ESP-IDF `IDF_PYTHON` env usually provides it). If unavailable, report the blocker and give an actionable fix (install `pyserial` in that env or run manually with `idf.py -p <PORT> monitor`).
2. **Prepare a repeatable command set.** For non-trivial runs, write commands to a file (one per line, `#` comments/blank lines ignored) so the session is reproducible. Group related checks; choose per-session timeouts/idle windows to fit slow commands (network scans, ping, time sync).
3. **Run the session and capture evidence.** Execute the helper against the user-provided port and save a raw timestamped transcript. Example (run from repo root, adjust port/paths/timeouts):
   - `python scripts/hw_console_session.py --port COM3 --baud 115200 --command-file <commands.txt> --out <artifact-dir>/serial-session.log --timeout 15 --idle-ms 500`
   - Add `--cmd "..."` (repeatable) for ad-hoc commands; add `--reset` only if the user requested a reboot and it is safe. `--startup-delay` captures initial/boot output before commands.
   The helper opens the port, sends each command, captures output until idle/timeout, then closes the port and records the log (open → execute → close).
4. **Assess results from the transcript.** For each planned step, compare actual output in the log to the expected markers/behavior. Record pass/fail/inconclusive per step with the relevant excerpt or line context. Treat missing output, timeouts, or unexpected errors as failures/inconclusive — not pass.
5. **Report.** State: port/baud, firmware/build identity if known, artifact path(s), exact command(s) run, per-step outcomes, and any anomalies (resets, garbled/paste issues, disconnects). Follow root `AGENTS.md` / `global-gates`: **never claim a check passed unless it was executed and evidenced in the captured log**. Explicitly list remaining **manual/hardware-only** steps (e.g. touch/visual checks, lighting changes, button presses, environment-dependent cases) and ask the user to perform/report them when needed.

## Limits and safety

- **Serial session scope:** automates console I/O and log capture only. Physical actions, visual confirmation, and subjective observations remain manual.
- **Bounded execution:** use appropriate `--timeout`/`--idle-ms`; long-running or interactive commands may need larger values or a dedicated manual run.
- **Non-destructive by default:** do not run erase/flash/factory-reset/profile-changing/restart commands unless explicitly requested and called out in the plan; prefer read-only/status commands for generic checks.
- **Environment sensitivity:** network/time/BLE results depend on device environment; capture actual state in the log and avoid over-claiming.
- **Manual fallback:** if scripted capture is unsuitable, use `idf.py -p <PORT> monitor` interactively and save the transcript/output as evidence.

## Artifacts

- Raw session log (timestamped TX/RX) is the primary evidence. Add a short summary (`summary.md` or report section) mapping steps to outcomes when producing validation records (see `docs/reports/` for examples).
- Keep secrets out of committed artifacts where possible (e.g. redact SSIDs/credentials in shared reports if required).
