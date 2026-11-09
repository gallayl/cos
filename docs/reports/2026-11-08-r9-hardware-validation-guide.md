# R9 Hardware Validation Guide — Berry command-script resolver + `path` + `ping` pilot

**Date:** 2026-11-08  
**Scope:** R9 from `docs/plan/consolidated-improvement-plan.md` / spec in `docs/history/2026-11-01-improvements.md`  
**Status:** Implementation merged locally (uncommitted in workspace at time of writing); **real-hardware validation required** before closing R9 and before any native `ping` migration decision.

## Preconditions

- Build and flash firmware that includes:
  - `components/shell/src/shell_script.c/.h` (`path` command, `/flash/.cos_path` persistence, resolver, `shell_script_try_exec`, `shell_dispatch_expanded_line`)
  - `components/shell/src/shell_input.c` interactive hook (alias/native dispatch, then script resolver on `ESP_ERR_NOT_FOUND`)
  - `components/shell/src/shell.c` calling `shell_script_init()`
  - `storage/flash_data/scripts/ping.be` pilot (`main(args)`, `net.ping`)
- `/flash` must be writable/mounted (config persistence target: `/flash/.cos_path`).
- Optional but recommended: SD card mounted if you want to validate non-`/flash` path/search dirs (e.g. `/sdcard/scripts`) and an active Berry UI app from SD.
- Network required only for the live `ping` pilot checks (R9-HW8). Offline checks (R9-HW1–7, R9-HW9) do not require connectivity.
- Record device/firmware/build info and serial logs for the progress log.

## What R9 does (behavior summary)

- **Interactive dispatch order:** alias expansion → native `esp_console_run` → if `ESP_ERR_NOT_FOUND`, split expanded line and try script resolver.
- **Native precedence:** registered native commands always win; scripts never shadow them.
- **Bare-name lookup:** `foo` → `foo.be` (or `foo.be` if already `.be`), searched in **CWD first**, then configured path dirs in order. First match wins. CWD is not stored in config.
- **Explicit path:** if the command token contains `/` (e.g. `./ping.be`, `/flash/scripts/ping.be`), resolve relative to CWD using virtual-path rules; **no search fallback**. Missing explicit path reports `<token>: script not found` (not `Unknown command`).
- **`path` config:** `/flash/.cos_path`, one normalized absolute directory per line. Max **8** configured entries. `path add` validates the directory exists **now** and reports persistence failures (state rolled back on save failure). Unavailable dirs later do not block later search entries.
- **Script execution:** `berry_runtime_exec_file` in shared Berry VM (VM mutex serialized). Command-script path does **not** close/hide an active Berry UI app or change UI app lifecycle. Script `main(args)` receives tokenized args (excluding command name) and returns int status (0 success, nonzero failure).
- **`ping` pilot:** `storage/flash_data/scripts/ping.be` preserves contract `Usage: ping <host> [timeout_ms]`, default 1000 ms, timeout 100–3000 ms, success `Reply from <address>: time=<rtt> ms`, failure `ping: <host> failed (<error>)`. **Native `ping` remains registered** until pilot passes target+host tests and migration is deliberate — so bare `ping` is still native; validate pilot via explicit path or `berry exec`.

## Test assets (create on device as needed)

You can create small command scripts with `echo`/editor/cat workflows already available on device, or place them under `/flash/scripts/` or `/sdcard/scripts/`. Example helper scripts:

**`/flash/scripts/hello.be`**
```berry
def main(args)
    print("HELLO argc=" + str(size(args)))
    var i = 0
    while i < size(args)
        print("arg[" + str(i) + "]=" + args[i])
        i = i + 1
    end
    return 0
end
```

**`/flash/scripts/fail42.be`**
```berry
def main(args)
    print("FAIL42")
    return 42
end
```

**`/sdcard/scripts/sdhello.be`** (optional, if SD mounted)
```berry
def main(args)
    print("SDHELLO")
    return 0
end
```

**`/flash/scripts/nomyname.be`** used only for CWD/path-order checks (copy/rename as needed).

## Checklist

| ID | Area | Result | Notes |
|---|---|---|---|
| R9-HW1 | `path` list/add/remove/clear basics | ✅ Pass (hw partial; deterministic cases unit-tested) | 2026-11-08 hardware: add/remove/clear/persisted. Host `test_shell_script` (19/19) covers idempotent add path, remove non-configured → error/config unchanged, bad arity/unknown subcommand → error, list/no-args success. |
| R9-HW2 | `path` persistence across reboot (`/flash/.cos_path`) | ✅ Pass | 2026-11-08 hardware: path list persisted across restart. |
| R9-HW3 | `path` validation/limits/errors | ✅ Pass (hw partial; deterministic cases unit-tested) | 2026-11-08 hardware: missing dir rejected. Host `test_shell_script` covers max 8 dirs (9th rejected, keeps 8), bad arity/usage, remove-missing/config-unchanged. Forcing real storage persistence failure not practical. |
| R9-HW4 | Bare-name script resolution (CWD, then path dirs, order) | ✅ Pass | 2026-11-08 hardware: CWD/path resolution confirmed (`be_ping`/`be_ping.be` from CWD and configured `/sdcard/scripts`). User reports: CWD-first, multi-dir order, and unknown command preserved — all passed. |
| R9-HW5 | Native precedence (bare `ping`/other natives still native) | ✅ Pass | 2026-11-08 hardware: user confirmed native `ping` takes precedence. |
| R9-HW6 | Explicit script paths (absolute/relative, no fallback) | ✅ Pass (core cases) | 2026-11-08 hardware: relative explicit paths `../sdcard/scripts/be_ping` and `../sdcard/scripts/be_ping.be` ran. `../sdcard/scripts/nope.be` → `../sdcard/scripts/nope.be: script not found` (not `Unknown command`, no fallback). Absolute explicit path/explicit-directory edge case not explicitly reported but core contract validated. |
| R9-HW7 | Quoted args, status propagation, usage/error output | ✅ Pass (core cases unit-tested; hw spot checks optional) | 2026-11-08 hardware: usage output seen; quotes confirmed. Host 	est_shell_script covers quoted args with spaces preserved, status propagation (7), Berry exec error reported, unknown remains not found, explicit missing/dir reported as script/path errors. |
| R9-HW8 | `ping.be` pilot contract via explicit path / `berry exec` | ❌ Blocked (script fixed; C++ corrected in workspace — reflash needed) | 2026-11-08 #1: `be_ping.be 127.0.0.1 1000` → `'map' object has no attribute 'ok'`. **Actual cause:** script used dot access on a plain Berry map (`result.ok`); `net.ping` returns a `map` instance and Berry maps require bracket access (`result["ok"]`). Original `native_net_ping` C++ was **correct** (`be_newobject` → `[instance,.p]`, populate `.p`, `be_pop(1)` remove `.p`, return instance). **Mistaken interim change (reverted):** removed that `be_pop`, which returned raw `.p` and caused `print(r)=(unknown value)`, `type(r)=map`, `r["ok"]` → "does not support subscriptable" (pingprobe confirmed). **Now fixed in workspace:** `native_net_ping` restored to pop `.p`/return instance; stock scripts fixed to bracket access (`storage/flash_data/scripts/ping.be`, `storage/ts/scripts/ping.ts`, `list_files`/`fetch_json`/`wget`). **Action:** rebuild+reflash firmware with restored `native_net_ping`, ensure SD/flash script uses bracket access, retest full pilot contract via explicit path/`berry exec`. Native `ping 192.168.0.1` works (HW5). |
| R9-HW9 | No active Berry UI app disturbed by command-script execution | 🔄 Partial | 2026-11-08 hardware: breakout started/closed with `berry run` using full path and relative path. The required case — running a **command script while a Berry UI app is active** without disturbing the app — not yet tested. |

## Hardware results (2026-11-08, partial)

User-reported validation (device details/log transcript not attached in chat except excerpts below):

1. **R9-HW5 ✅** Native `ping` takes precedence.
2. **R9-HW6/R9-HW4 partial:** with `/sdcard/scripts` on path, `path clear` then `be_ping` from `/sdcard/scripts` CWD ran Berry script (`BERRY!!! Usage: ping <host> [timeout_ms]`), confirming CWD/bare-name resolution. From `/flash`, `../sdcard/scripts/be_ping` and `../sdcard/scripts/be_ping.be` ran successfully (relative explicit paths). Missing-explicit-path/no-fallback and full order cases still needed.
3. **R9-HW8 still blocked (script must use map bracket access):** after rebuild/reflash with `native_net_ping` fix, `be_ping 192.168.0.1` → `'map' value has no attribute 'ok'`; `be_ping` no-args usage OK; native `ping 192.168.0.1` OK. The custom/stock script used `result.ok` (dot) on a plain Berry map — must use `result["ok"]`/`result["address"]`/`result["rtt_ms"]`/`result["error"]`. Repo pilot/scripts fixed; redeploy updated script to device and retest full ping contract.
4. **R9-HW9 partial:** breakout starts/closes with `berry run` (full and relative path); `ping` worked while running breakout (good signal). Still need explicit check: run a **command script** via explicit path while a Berry UI app is active and confirm app not disturbed.
5. **R9-HW3 partial:** `path add /sdcard/asdasd` → `path: '/sdcard/asdasd' is not an available directory` (correct).
6. **R9-HW7 partial:** quoted args still work (user confirmed). Full status/usage/script-error cases still useful.

**Still required for R9 closure:** R9-HW3 limits/bad-arity/persistence-failure cases as feasible, complete R9-HW4 (CWD-first vs configured dir, multi-dir order, unknown command), R9-HW6 missing explicit path/no fallback (+ absolute/dir edge cases), R9-HW7 quoting/args/status/script-error cases, **reflash+retest R9-HW8** ping pilot via explicit path/`berry exec` after `net.ping` fix, R9-HW9 script-while-UI-app-active. After full hardware pass, decide/record native `ping` migration. Also consider auditing other `berry_network` map-return paths for the same `be_pop`/`be_return` stack issue (not changed here except `native_net_ping`).

## R9-HW1 — `path` list/add/remove/clear

1. `path` (or `path list`) with no config → expect `(empty)`.
2. `path add /flash/scripts` (or another existing dir) → expect success (no output / status 0). `path` → shows `/flash/scripts`.
3. Add same dir again → expect no duplicate (idempotent). `path` still shows it once.
4. Add a second existing dir (e.g. `/sdcard/scripts` if available) → `path` shows both in insertion order.
5. `path remove /flash/scripts` → removed; `path` reflects removal. Removing a non-configured normalized path → `path: '<path>' not found` and nonzero status.
6. `path clear` → `path` shows `(empty)`.
7. `path bogus` / bad arity → `Usage: path [list|add <directory>|remove <directory>|clear]`.

**Pass:** output/status match; no crash; config reflects operations.

## R9-HW2 — Persistence across reboot

1. `path clear` (clean baseline).
2. `path add /flash/scripts` (and optionally a second dir).
3. Confirm `cat /flash/.cos_path` (or equivalent) contains normalized absolute dir(s), one per line.
4. Reboot the device.
5. After boot: `path` → shows the same configured dirs in the same order.
6. `path remove ...` / `path clear`, reboot again → changes persist.

**Pass:** `/flash/.cos_path` survives reboot and is loaded by `shell_script_init()`.

## R9-HW3 — Validation, limits, errors

1. `path add /does/not/exist` → `path: '/does/not/exist' is not an available directory` (or normalized form) and nonzero status; config unchanged.
2. `path add not-absolute-relative-form` behavior: implementation normalizes via `shell_resolve_relative`; verify it resolves relative to CWD and validates existence. Record actual output if surprising.
3. Add > 8 distinct existing dirs (or fill to limit) → 9th add reports `path: maximum of 8 directories reached` and nonzero; existing 8 unchanged.
4. (Optional, if safe to test) simulate persistence failure? Not required on hardware; host tests cover rollback on save failure. If observed in logs, confirm command reports `path: could not save configuration: ...` and in-memory state is rolled back.

**Pass:** invalid/missing dirs rejected; max 8 enforced; persistence errors are reported (not silent success).

## R9-HW4 — Bare-name resolution (CWD first, then path dirs)

Setup:
```text
path clear
path add /flash/scripts
# Ensure /flash/scripts/hello.be exists (from assets)
```

1. From a directory **not** containing `hello.be`, run `hello` → resolves via `/flash/scripts/hello.be`; prints `HELLO argc=0`.
2. Copy/create a different `hello.be` in CWD (if writable) that prints a distinguishable marker (e.g. `HELLO-CWD`). Run `hello` from that CWD → **CWD version wins**.
3. If two configured path dirs both contain same script name, first configured dir wins (insertion order). Verify by ordering dirs and placing distinct scripts.
4. `unknownscriptxyz` (no native, no script) → existing `Unknown command` behavior preserved.

**Pass:** lookup order is CWD, then configured dirs in order; first match wins; unknown remains `Unknown command` for non-explicit input.

## R9-HW5 — Native precedence

1. `ping` (no args) — **native** command behavior is expected (native `ping` remains registered). It must **not** silently run `/flash/scripts/ping.be` as the bare command while native is still present.
2. `help` / `ls` / `echo` / `berry` (pick a few natives) — confirm unchanged native behavior.
3. (Optional) Place a script named after a native in CWD/path (e.g. `ls.be`) — bare `ls` must still run native `ls`.

**Pass:** native commands always take precedence over same-named scripts.

## R9-HW6 — Explicit script paths (no fallback)

1. `/flash/scripts/hello.be` → runs that script.
2. `cd /flash/scripts` then `./hello.be` (or relative form supported by resolver/CWD) → runs script resolved relative to CWD.
3. `/flash/scripts/does_not_exist.be` → `<token>: script not found` (reported by resolver) and **not** `Unknown command`; no search-path fallback.
4. Explicit path to a directory (if resolvable to an existing directory) → should be treated as not a runnable script / error (implementation checks `vfs_is_directory`); record actual message.
5. Token containing `/` that is not found must not fall through to bare-name search.

**Pass:** explicit paths resolve only the specified path; missing explicit path gives script/path error, not `Unknown command`.

## R9-HW7 — Args, quoting, status, usage/error output

Using `/flash/scripts/hello.be` (explicit path to avoid native ambiguity):
1. `/flash/scripts/hello.be one two three` → `argc=3`, args in order.
2. Quoted args (shell quoting supported by existing tokenization): e.g. `/flash/scripts/hello.be "hello world" second` → one arg contains space (`hello world`), ordering preserved. Use the quoting style supported by the device shell; record exact input/output.
3. Empty-arg behavior: only if supported by existing shell tokenization; record; not required to invent new quoting semantics.
4. `/flash/scripts/fail42.be` → prints `FAIL42`, returns status 42 (observe prompt/status handling if shell surfaces it).
5. Missing `main`, Berry exception, or compile/runtime error in a script: create a small broken script and confirm resolver reports `<script>: <error>`/failure status and does not print `Unknown command`.
6. Berry initialization/runtime errors are reported as script/command failures (hard to force on healthy device; host tests cover some paths).

**Pass:** tokenized args delivered to `main(args)`; status propagated; script/resolver errors reported clearly.

## R9-HW8 — `ping.be` pilot contract (explicit path)

> Requires network connectivity to a reachable host. Native `ping` is still registered, so use **explicit path** (or `berry exec`) for the pilot.

1. `/flash/scripts/ping.be` (no args) → `Usage: ping <host> [timeout_ms]`, status nonzero.
2. `/flash/scripts/ping.be example.com` → success form `Reply from <address>: time=<rtt> ms` (address/RTT vary), status 0.
3. `/flash/scripts/ping.be example.com 500` → same success form with explicit timeout.
4. `/flash/scripts/ping.be example.com 99` and `/flash/scripts/ping.be example.com 3001` → `ping: timeout must be 100-3000 ms`, status 1.
5. `/flash/scripts/ping.be example.com abc` → timeout validation error, status 1.
6. `/flash/scripts/ping.be definitely.invalid.example` (or unreachable host) → `ping: <host> failed (<error>)`, status nonzero.
7. Alternative validation path: `berry exec /flash/scripts/ping.be example.com 1000` should produce equivalent script behavior (same `main(args)` contract).

**Pass:** output/status match current `ping` contract within Berry/script constraints; uses `net.ping`. Record that bare `ping` remained native during this validation.

## R9-HW9 — Active Berry UI app not disturbed

This validates the spec requirement: command scripts run in shared Berry VM but **must not** close an active UI app, hide text console, create UI task, or change app state.

1. Start a known Berry UI app: e.g. `berry run /sdcard/apps/breakout.be` or `berry run /flash/apps/<app>.be` (use an app that is visibly active/responds to input).
2. While the app is active, switch to/issue a shell command that runs a **command script** via explicit path, e.g. `/flash/scripts/hello.be from-shell` or `/flash/scripts/fail42.be`. (Avoid relying on bare-name lookup if it could be ambiguous; explicit path is clearest.)
3. Observe:
   - Script output appears as expected.
   - The Berry UI app remains active/running (not closed/hidden by the script execution). Verify by interacting with the app or by expected on-screen state.
4. `berry close` (when appropriate) still closes the app normally afterward.
5. (Optional) Compare with `berry exec <file>` behavior for the same script — both use `berry_runtime_exec_file`; command-script path should have equivalent non-disruptive VM behavior.

**Pass:** running a command script does not disturb the active Berry UI app. Any observed disruption is a blocker — capture logs and R5-style heap/runtime context.

## Evidence to capture

- Device/board, firmware build id/commit (local workspace state), flash/storage notes.
- Serial console transcript for each checklist section (at minimum pass/fail lines).
- Contents of `/flash/.cos_path` before/after reboot test.
- For R9-HW8: network environment note (Wi-Fi connected, target host reachable/unreachable as intended).
- For R9-HW9: which UI app used and how “still active” was confirmed.

## After hardware pass

1. Update `docs/plan/consolidated-improvement-plan.md` (R9 row/progress, progress log) with results/evidence links.
2. Update `docs/history/2026-11-01-improvements.md` R9 status/progress/follow-ups (mark implemented items, record hardware validation).
3. Decide and record native `ping` migration: remove or make unreachable native registration only when script lookup is active and pilot validated; update `README.md` command docs accordingly.
4. Consider QEMU/integration coverage for resolver/`path` (seed a test `.be` command script in the integration app if useful).
5. Re-run/record relevant host tests if code changes after hardware findings (`test_shell_script`, `test_shell`, `test_shell_berry`).

## Known local validation (pre-hardware)

- Host build in `test/build-r9`: `test_shell_script` 14/0, `test_shell` 63/0, `test_shell_berry` 26/0.
- Target ESP-IDF build/flash and all R9-HW* checks above: **pending**.
