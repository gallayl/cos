# R7 Hardware Validation Guide — Fetch `include_body` (P3)

**Date:** 2026-11-08  
**Item:** R7 — Fetch `include_body` option for `net.fetch` (`docs/plan/consolidated-improvement-plan.md`, `docs/history/2026-11-06-memory-pressure-fixes.md` P3)  
**Status:** Implementation, SDK/docs update, Berry syntax validation, and user hardware validation complete (2026-11-08). Live check (historical, pre-R21): `berry run body.be http://ip-api.com/json` (formerly `berry run`) reported good. Optional heap comparison still useful if deeper memory evidence is needed.

## What changed

- `net.fetch(url, options)` accepts optional `include_body: bool` (default `true`).
- When `include_body` is `false`, the returned `body` field is an empty string. This avoids creating a Berry string copy of the raw HTTP response body.
- JSON parsing is unchanged: with `response_type: "json"`, `net.fetch` still parses the response and returns `json`/`json_ok` as before.
- Existing scripts are unchanged because the default is `true`.
- Invalid `include_body` type (non-bool) is rejected as `ESP_ERR_INVALID_ARG` during option parsing, before the network request is queued.

## Host validation completed

- `scripts/validate-berry-scripts.ps1`: passes for `storage/flash_data/tests/r7/include_body.be` (and full `storage/flash_data` set when run).
- Frontend tests: `npx vitest run` in `frontend/` (covers compiler/IDE; `network.d.ts` change is type-only).

Firmware/network runtime behavior cannot be fully validated on the host. Hardware validation is required.

## Compatibility notes

- **Default unchanged:** omit `include_body` or pass `true` to keep the existing `body` behavior.
- **Memory optimization use:** `net.fetch(url, { response_type: "json", include_body: false })` keeps `json`/`json_ok` and sets `body` to `""`.
- **Non-JSON/text fetches:** `include_body: false` also returns an empty `body`; use only when you intentionally do not need the raw response.

## Prerequisites

- Firmware flashed with R7 changes (`components/berry_lang/src/berry_network.cpp`, SDK/docs if you use the IDE).
- Serial console connected.
- SD card or flash path containing `tests/r7/include_body.be`.
- For live network checks: Wi-Fi connected and a reachable endpoint returning small JSON (HTTP or HTTPS, depending on device/cert bundle state; see R2/R8 follow-ups for HTTPS constraints).

## Procedure

`include_body.be` is a **command script** (`def main(args)`). R21: use `berry run` (unified; passes args to `main(args)`).

### 1. Offline argument validation (no network required)

```text
berry run /sdcard/tests/r7/include_body.be
```

Use `/flash/tests/r7/include_body.be` if deployed to flash. No URL argument is needed; the script still rejects invalid `include_body` types before making network requests.

**Expected:**
- `R7 arg validation: invalid include_body type rejected`
- `R7 arg validation: bool values accepted`
- `R7 ARG OK (no URL provided; network behavior not checked)`

**Failure to investigate:** `ESP_ERR_INVALID_ARG` not returned for a non-bool `include_body`, bool `include_body` rejected as invalid arg, crash, or wrong result shape.

### 2. Live JSON fetch behavior (network required)

Choose a stable JSON endpoint you can reach from the device. Example (only if reachable on your network):

```text
berry run /sdcard/tests/r7/include_body.be https://httpbin.org/json
```

Or use a local/LAN HTTP endpoint returning `application/json`.

**Expected:**
- `with_body ... json_ok true body_len > 0` (default behavior)
- `no_body ... json_ok true body_len 0` and parsed `json` still present
- `R7 OK`

**Failure to investigate:** `include_body=false` returns non-empty body; `json_ok`/`json` missing when parsing succeeded; regression in default body behavior.

### 3. Existing script regression

Run a script that already uses JSON fetch without `include_body`:

```text
berry run /flash/scripts/fetch_json.be <json-url>
```

Or run `storage/flash_data/tests/r2/json_fetch.be` after editing its URL for your environment.

**Expected:** unchanged behavior (`body` still printed when present; JSON parsing unchanged).

### 4. Optional heap sanity check

This is best-effort (Berry GC/network heap behavior is noisy). Capture before/after around repeated JSON fetches:

```text
berry profile
berry gc
berry profile
```

Compare runs with and without `include_body: false` on the same endpoint/payload. The primary expected win is avoiding the Berry string copy for the raw body; it may not show as a large persistent delta because the network layer still buffers the response.

## Pass criteria

- [ ] Offline argument validation prints `R7 ARG OK`.
- [ ] Live JSON fetch shows default `body_len > 0` and `include_body=false` `body_len == 0` with `json_ok true` and `json` present.
- [ ] Existing fetch/JSON scripts behave unchanged without `include_body`.
- [ ] No crashes, option-parsing regressions, or JSON parse regressions.

## If issues occur

Record serial logs, the exact `berry run` command, URL class (HTTP/HTTPS/local), and `berry profile` if relevant.

- **Option rejected:** confirm firmware has R7 `known_fetch_field`/`include_body` parsing in `berry_network.cpp`.
- **Body still present with `include_body=false`:** check final result path after JSON parsing and `push_fetch_result` arguments.
- **JSON parsing regressed:** R7 must not change the JSON parse path; inspect trimmed-body parsing and `json_ok`/error handling.
- **HTTPS/connectivity failures:** may be environmental or related to R2/R8 follow-ups (`PSA_ERROR_INSUFFICIENT_MEMORY`, timeout cleanup); distinguish from R7 option handling.

After validation, update `docs/plan/consolidated-improvement-plan.md` R7 progress/log and, if desired, record evidence in `docs/reports/2026-11-08-r7-hardware-results.md`.
