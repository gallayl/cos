# R22 M3 Hardware Validation — `light` command-script migration

**Date:** 2026-11-15  
**Branch/PR:** `feat/r22-m3-light-script` (pending)  
**Scope:** Validate migrated `light` command behavior on supported CYD hardware after moving from native console registration to Berry command script + structured `light.read()` API.

## Preconditions

- Firmware built/flashed from this branch (includes `light` native Berry module, `/flash/scripts/light.be`, and removed native `light` console registration).
- Device booted normally with light sensor initialized (see boot logs / normal sensor behavior).
- Stock scripts discoverable by resolver as in your environment (typically `/flash/scripts` on CWD/path per R9/R22 conventions). If needed: `path add /flash/scripts`.

## Automated/local checks completed (evidence in PR)

- Baseline CI on `main` (b094673, #35) confirmed green before branch: Unit Tests, Doxygen, Frontend, Format Check, Build & Lint, Integration Tests ✅ (`gh run list --branch main`).
- Local validation for this change: see PR/commit notes for exact `make format-check`, `make test`, frontend checks (if run), and any firmware build/lint results or environment blockers.

## Validation steps

### 1. Basic command resolution and success path

1. From shell, run: `light`
2. **Expected:** prints `Light sensor: <value>` where `<value>` is an integer in range 0–4095 (raw ADC). Command returns success (exit status 0 if observable via your workflow).
3. Repeat a few times under different ambient lighting (cover sensor / expose to light).
4. **Expected:** values change appropriately (sensor is inverted in brightness logic elsewhere, but raw read contract remains 0–4095).

### 2. Explicit script path (resolver parity)

1. Run: `/flash/scripts/light.be` (or the deployed stock script path on your device)
2. **Expected:** same output/behavior as `light`.

### 3. Structured Berry API (script/runtime sanity)

If Berry REPL/app execution is available in your validation flow, confirm:
- `cos_import("@cos/light")` / module load succeeds and `light.read()` returns a map: `{ "ok": true, "value": <int> }` on success.
- No console-output parsing is required.

### 4. Error/unavailable behavior (if reproducible)

If the sensor read can be made to fail/unavailable in your setup:
1. Run `light`
2. **Expected:** prints `light: read failed` and returns failure (exit status 1).

> Note: inducing a true sensor failure may not be practical without hardware changes; record N/A if not reproducible, but confirm normal success path.

### 5. Regression spot-checks

- `led` and `screen` commands still behave as before (native, unrelated).
- Other migrated script commands (e.g. `uptime`, `info`, `mem`, `nslookup` if applicable) still resolve correctly.

## Record results

Please capture:
- Device/board, firmware image/commit, date.
- Sample `light` outputs under different lighting.
- Pass/fail for each step above and any deviations.
- Logs/screenshots if helpful.

Update `docs/plan/2026-11-12-berry-command-script-migrations.md` (M3) and `docs/plan/consolidated-improvement-plan.md` (R22 progress/evidence) with hardware validation outcome when available.
