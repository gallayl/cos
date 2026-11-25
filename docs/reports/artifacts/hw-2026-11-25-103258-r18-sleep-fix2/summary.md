# R18 sleep fix on-device validation (COM3)

- Date: 2026-11-25 10:32:58 (local)
- Device: ESP32 CYD (COM3), normal shell/no-app mode
- Firmware: built from `feat/r18-async-await-continuation` with bootstrap+wrapper fix (`cos.bin` 0x1ce1d0)
- Command: `python scripts/hw_console_session.py --port COM3 --baud 115200 --reset --startup-delay 18 --timeout 20 --idle-ms 3000 --command-file docs/reports/artifacts/hw-2026-11-25-103258-r18-sleep-fix2/cmd.txt --out docs/reports/artifacts/hw-2026-11-25-103258-r18-sleep-fix2/log.txt`
- Redaction: `log.redacted.txt` masks MAC/IP/SSID.

## Required outcomes

| Outcome | Result | Evidence |
|---|---|---|
| `type(Promise)=="class"` | PASS | `PROMISE_TYPE=class` |
| `type(system.sleep)=="function"` | PASS | `SLEEP_TYPE=function` |
| `system.sleep(ms)` returns Promise (not int) in normal shell/no-app mode | PASS | `SP_TYPE=instance`, `SP2_TYPE=instance` |
| Promise callback fires | PASS | `SLEEP_OK:nil`, `SLEEP2_OK:nil` |
| No background `ASYNC_TIMER_LIMIT` fill-up in idle shell | PASS | No `ASYNC_TIMER_LIMIT`/`Unhandled` in log; `Async: timers=0 microtasks=0` at baseline/after sleeps/idle/after close |
| `berry profile` async counters sane (no unbounded growth) | PASS | `timers=0 microtasks=0` across all profile samples |
| `berry close` / teardown sanity OK | PASS | `berry close` returned prompt; post-close profile shows `timers=0 microtasks=0`, GC initialized |

## Notes

- One unrelated marker eval (`print('IDLE_CHECK=ok')`) returned `ESP_FAIL`; subsequent profile/close were healthy. Not a required outcome.
- Raw log contains WiFi/BLE identifiers; use `log.redacted.txt` for sharing.
