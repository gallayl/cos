# On-device validation summary — low-memory display v1 (PR #113)

## Session

- **Date:** 2026-11-23
- **Port/Baud:** COM3 / 115200
- **Helper:** `python scripts/hw_console_session.py` (pyserial 3.5)
- **Reset:** yes (`--reset --startup-delay 5` on first session)
- **Firmware (as reported by boot log):** `cos`, app version `3a20c89-dirty`, compile `Nov 23 2026 10:00:14`, ELF SHA256 `147207949...`, ESP-IDF `v6.1`, chip ESP32 rev v3.1
- **Artifacts:**
  - `docs/reports/artifacts/hw-2026-11-23-low-memory-display-com3/serial-session.log` (boot + initial smoke)
  - `docs/reports/artifacts/hw-2026-11-23-low-memory-display-com3/serial-session-smoke2.log` (follow-up; berry `-e` misuse recorded)
  - `docs/reports/artifacts/hw-2026-11-23-low-memory-display-com3/serial-session-smoke3.log` (screen status, berry eval, bad for-loop syntax recorded)
  - `docs/reports/artifacts/hw-2026-11-23-low-memory-display-com3/serial-session-smoke4.log` (scroll + ANSI-attempt + svc_mem)
  - Command files: `commands.txt`, `commands-smoke2.txt`, `commands-smoke3.txt`, `commands-smoke4.txt`

## Automated console results (evidence-based)

| Step | Result | Evidence |
|---|---|---|
| Boot/reset and reach prompt | **PASS** | `serial-session.log`: boot banner, `Display initialized (240x320)`, `COS/flash>` prompt |
| `svc_mem` at early boot/console-ready | **PASS** (captured) | `serial-session.log`: default_free 59876, default_min_free 59876, default_largest_block 59392; internal_largest_block 59392 (early snapshot before full steady-state) |
| `services` | **PASS** | `serial-session.log`: http_server/websocket/wifi/bluetooth/network/time_sync reported running/idle states as expected |
| `help` includes console commands | **PASS** | `serial-session.log`: `clear`, `display_mode`, `screen`, `svc_mem`, `services`, etc. present |
| `display_mode text` | **PASS** | `serial-session.log`: "Display mode: text (already active)"; smoke2: command accepted |
| `clear` | **PASS** (command accepted) | `serial-session.log` and smoke2: command executed, prompt returned. **Visual clear not verified by agent.** |
| `screen brightness` / `screen rotation` | **PASS** | `serial-session-smoke3.log`: `Brightness: 128 (manual)`, `Rotation: 3` |
| Shell interaction / berry eval | **PASS** | `serial-session-smoke3.log`: `berry eval "print('SMOKE-OK')"` -> `SMOKE-OK` |
| Scroll-producing output | **PASS** (no crash/hang) | `serial-session-smoke4.log`: `for i:0..35 print('scroll-'+str(i)) end` produced `scroll-0`..`scroll-35`, prompt returned |
| `svc_mem` after representative output | **PASS** (captured) | `serial-session-smoke4.log`: default_free 21364, default_min_free 7268, default_largest_block 8192, internal_largest_block 8192/32768; no crash |
| No panic/reset/error indicating display failure | **PASS** (within captured sessions) | No panic/backtrace/reset observed in captured logs; commands returned normally |

## Not verified / inconclusive

| Item | Status | Reason |
|---|---|---|
| `ls /flash` | **INCONCLUSIVE / unrelated failure** | `serial-session.log`: `berry: /sdcard/scripts/ls.be: run(args) failed: 'string' value has no method 'charAt'`, `ls: ESP_FAIL`. Pre-existing device script issue; not caused by display change. |
| ANSI color/cursor/attribute rendering on LCD | **NOT VERIFIED** | Smoke4 ANSI attempt emitted literal `1b[...]` text due to escaping through command transport, not real ESC bytes. Visual ANSI rendering on CYD screen not confirmed by agent. |
| Visual console correctness (text, colors, cursor, scrollback, clear, resize/rotation/font-size behavior) | **NOT VERIFIED (manual required)** | Serial capture cannot confirm on-screen rendering. |
| Touch/shell interaction feel, render latency/CPU impact | **NOT VERIFIED (manual required)** | Subjective/physical observation required. |
| Before/after row-sprite allocation comparison | **NOT VERIFIED** | No baseline old-firmware measurement captured in this session; `svc_mem` does not expose text_console row-sprite allocation. Code/config review indicates default path has no row sprite (`CONFIG_COS_DISPLAY_LOW_MEMORY_CONSOLE=y`), but on-device before/after memory comparison remains unproven. |
| Rotation/font-size/resize smoke beyond command acceptance | **NOT VERIFIED** | No font-size command exposed; rotation queried only; visual behavior not confirmed. |

## Manual steps required (user)

1. Confirm on CYD screen: text readable, no corruption, clear actually clears, scrolling/scrollback behaves correctly.
2. Confirm ANSI/cursor/colors/attributes visually (run a known ANSI test or `printf` real ESC sequences on device console).
3. Confirm rotation/font-size/resize behavior as supported by firmware/UI; note any deviation.
4. Observe render latency/CPU impact during heavy console output vs prior behavior.
5. If before/after memory comparison is required: flash baseline build (or record prior known measurements), capture `svc_mem`/heap watermark/largest block at same points (boot/console-ready/after scrollback allocation/after representative output), and compare. Record exact builds/commands.

## Limitations

- Serial console automation only; no physical/visual confirmation.
- `svc_mem` text_console metrics are reported as unknown/not exposed; cannot directly confirm row-sprite absence at runtime.
- Device had Wi-Fi connected and Bluetooth running during smoke; heap numbers reflect full default boot state, not isolated display-only delta.
