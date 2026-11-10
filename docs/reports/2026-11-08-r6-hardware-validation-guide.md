# R6 Hardware Validation Guide — On-Demand Native Module Loading

**Date:** 2026-11-08  
**Item:** R6 — Firmware on-demand native module loading (`docs/plan/consolidated-improvement-plan.md`)  
**Status:** Implementation complete in firmware source; **hardware validation required before closing R6 acceptance**.

## What changed

- `berry_runtime_init()` no longer eagerly loads `console`, `fs`, `net`, and `ui` native modules.
- Startup installs lightweight **lazy proxy globals** for those modules (small dynamic Berry modules with a `member` hook).
- On first bare-global attribute access (for example `ui.label(...)`, `net.fetch(...)`), the proxy:
  1. loads the real native module from `be_modtab.c` via `be_getmodule()`;
  2. replaces the global with the real module;
  3. returns the requested attribute.
- `cos_import('@cos/*')` and Berry `import <module>` still load modules on demand and bypass the proxy.
- `json` remains on-demand via R1 (`berry_runtime_ensure_json()`); R6 does **not** install a lazy global proxy for `json`.

## Host validation already completed

- `scripts/validate-berry-scripts.ps1`: **23/23** `.be` files compiled (includes `tests/r6/lazy_modules.be`).
- Host unit tests: `test_shell_berry` **26/26**, `test_berry_source_buffer` **2/2**, `test_shell` **63/63**.

These do not exercise the real ESP32 Berry VM/native module load path. Hardware validation is required.

## Compatibility rules (recorded)

- **Preferred (TS):** `import ui from '@cos/ui'`, `import net from '@cos/net'`, etc. TS-compiled COS scripts/apps already follow this and should be unaffected.
- **Preferred (hand-written Berry):** `import fs` / `import net` / `import ui` / `import console` / `import json`.
- **Legacy compatibility:** bare `console.*`, `fs.*`, `net.*`, `ui.*` still work through lazy proxies; the **first** attribute access loads the real module.
- **`json` note:** bare global `json` is not preinstalled. Use `import json`, or use runtime-provided paths that already ensure JSON (`fs.read_json`/`fs.write_json`, `net.fetch(..., response_type="json")`).

## Prerequisites

- Flash firmware containing R6 changes.
- SD card available if using `/sdcard/...` paths (scripts fall back to `/flash/...` where written).
- Serial console connected.

## Procedure

### 1. Clean boot / baseline snapshot

```text
berry profile
```

Record: Berry managed bytes, step rate (default 200), system/internal heap, largest blocks, watermarks. Confirm boot completes without module-load errors. Note: R6 installs 4 tiny lazy proxy modules at init; compare against a pre-R6 baseline if available (R2/R5 reports) to assess net startup impact.

### 2. Deploy/run R6 probe script

Deploy `storage/flash_data/tests/r6/lazy_modules.be` to flash/SD as appropriate, then run:

```text
berry profile
berry run /sdcard/tests/r6/lazy_modules.be
berry profile
```

(Use `/flash/tests/r6/lazy_modules.be` if deployed to flash.)

**Expected output includes:**

- `R6 lazy module probe`
- `R6 import fs write true` (or equivalent success path)
- `R6 import fs read true r6`
- `R6 cos_import net base_url type ...`
- `R6 bare ui.KEY_ENTER 40` (HID Enter key code)
- `R6 bare console.font_size ...` (numeric font size)
- `R6 import json dump string true`
- `R6 OK`

**Failure to investigate:** any `import_error`, `attribute_error`/missing member, crash, or hang.

### 3. TS-compiled app regression (real COS module usage)

Run shipped TS-compiled apps/scripts that use `cos_import`:

```text
berry run /flash/apps/tap_dash.be
```

Play briefly, exit, then:

```text
berry close
berry profile
berry run /flash/apps/breakout.be
```

Play briefly, exit, then:

```text
berry close
berry profile
```

**Expected:** normal app behavior, no module/import errors, no progressive leak pattern beyond known R5 baselines.

### 4. Script-path / `cos_import` command regression

Run representative TS-compiled scripts:

```text
berry run /flash/scripts/host_info.be
berry run /flash/scripts/hello.be
```

If network is available and safe to use, optionally exercise network/json paths already covered by R1/R2 (do not regress timeout/heap-collapse follow-ups).

**Expected:** normal output/errors consistent with environment (e.g. network unavailable errors are fine; module/import failures are not).

### 5. Bare-global legacy compatibility (ad hoc)

Create or run a temporary Berry script that uses bare globals without explicit imports, e.g.:

```berry
# bare-global compatibility smoke
print(fs.exists("/flash"))
print(ui.LEFT)
print(console.font_size())
```

**Expected:** values/booleans print; no "global not found"/member errors.

### 6. JSON R1 regression spot check

```text
berry run /flash/tests/r2/fs_json.be
```

If network is available, optionally rerun the R1/R2 fetch-JSON validation path. **Expected:** JSON FS/fetch behavior unchanged from R1.

### 7. Optional memory comparison (R2/R5 tie-in)

If you have pre-R6 numbers from the same build/config:

- Compare `berry profile` at clean boot (after init) and after first `ui`-using app launch.
- Confirm modules used only on demand: a non-UI/network script should not force UI/network module loading beyond proxy globals.
- Watch `berry_gc` managed bytes, largest free blocks, and watermarks.

Note: exact attribution is approximate because Berry GC/module caches and proxies have overhead. The acceptance target is "avoid unnecessary allocation/loading where feasible" and no regressions.

## Pass criteria (for closing R6 hardware validation)

- [ ] Boot and `berry profile` succeed with R6 firmware.
- [ ] `tests/r6/lazy_modules.be` completes with `R6 OK` and expected probe lines.
- [ ] TS-compiled apps (`tap_dash.be`, `breakout.be`) launch/exit normally.
- [ ] Representative `cos_import` scripts (`host_info.be`, `hello.be`) behave normally.
- [ ] Bare-global compatibility smoke test works for `ui`/`fs`/`console` (and `net` where practical).
- [ ] R1 JSON spot check still passes.
- [ ] No new crashes, import errors, or obvious heap/watermark regressions attributable to R6.

## If issues occur

Record serial logs, `berry profile` before/after, and the failing command/script. Likely areas:

- **Lazy proxy recursion/self-healing:** `member` hook loads module and replaces global; verify `member`/`setmember` behavior if a script accesses unusual attributes.
- **`json` global expectations:** R6 does not proxy `json`; scripts using bare `json.*` must `import json`.
- **VM lock/reentrancy:** proxy hook executes in Berry VM call path under existing runtime discipline; report deadlocks or races.
- **SDK global declarations:** TS global `ui`/`net`/`fs`/`console` types still exist for compatibility; runtime now uses lazy proxies, so behavior should be preserved — report mismatches.

After validation, update `docs/plan/consolidated-improvement-plan.md` R6 status/progress log and `docs/history/2026-11-07-ts-feature-extensions.md`.
