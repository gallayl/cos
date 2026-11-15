# R14 Hardware Validation Guide — Compiler Profiles, Source Maps, Runtime Error Mapping & Production Debug Evaluation

**Date:** 2026-11-10  
**Status:** Pending hardware validation (tooling/scripts prepared)  
**Scope:** `docs/plan/consolidated-improvement-plan.md` R14 — compiler profiles, debug metadata, source maps, editor/console diagnostics, editor runtime memory diagnostics, and `BE_DEBUG_RUNTIME_INFO`/prod-profile firmware evaluation follow-ups.  
**Why hardware/runtime validation is required:** R14 mapping UX depends on real Berry/device runtime output formats and line information; firmware debug/profile tradeoffs (`BE_DEBUG_RUNTIME_INFO`) and memory/runtime behavior require ESP32 target measurements. Frontend/compiler unit tests cover mapping/annotation logic with synthetic data, not real device traces.

## Prerequisites

- Supported COS hardware (ESP32/CYD target used by this repo) with current firmware flashed from this workspace.
- USB serial console access and a way to capture logs to file (recommended: `idf.py -p <PORT> monitor`, PuTTY/Tera Term logging, or `tio`/`screen` with logging).
- Device on the same network as the development browser for Web IDE + WebSocket console validation (or access via device-hosted web UI as used in your setup).
- Frontend dev/build served and able to reach the device (`/api/heap`, filesystem APIs, `/ws` console) with valid HTTP Basic auth credentials.
- Test Berry/TypeScript sources available on device storage (examples under `/flash/apps`, `/flash/tests`, `/sd` as applicable).
- (For firmware debug/profile evaluation) ESP-IDF environment configured, ability to build/flash firmware variants, and a record of commit/config used.

## Artifacts to collect (save under a dated folder, e.g. `docs/reports/artifacts/r14-YYYYMMDD/`)

- Serial log captures (`.log`/`.txt`) for each scenario.
- Browser console/network notes or screenshots for Web IDE and console behavior.
- For each compile: generated Berry (`.be`) when relevant, TypeScript source, and compile profile/metadata summary shown in editor.
- Exact firmware config/commit/build measurements for `BE_DEBUG_RUNTIME_INFO` comparison (if performed).
- A short results summary: pass/fail per checklist item, observations, and any raw device error strings that were not auto-annotated.

## Helper script

Use `scripts/r14-hardware-validation.ps1` to create an artifact folder, prompt for device/port info, optionally collect `idf.py` build-size output, and guide log capture/checklist execution.

```powershell
# Example (from repo root)
./scripts/r14-hardware-validation.ps1 -Port COM5 -DeviceIp 192.168.1.50 -OutDir docs/reports/artifacts/r14-$(Get-Date -Format yyyyMMdd)

# Size/firmware-eval helper mode (does not change config automatically)
./scripts/r14-hardware-validation.ps1 -FirmwareEval -IdfPy "idf.py" -OutDir docs/reports/artifacts/r14-firmware-$(Get-Date -Format yyyyMMdd)
```

The script is a guided checklist/logger aid; it does **not** modify firmware config, flash automatically, or claim validation passed.

---

## Part A — R14 source-map / runtime error mapping on real device output

**Goal:** Confirm how real Berry compile/runtime/device errors appear (serial + WebSocket console/editor surfaces) and whether R14 mappings annotate them correctly.

### A1. Preparation

1. Build/serve frontend and open the COS editor in browser; sign in with device credentials.
2. Confirm **dev profile** compile is active (default) and TypeScript diagnostics compile produces mappings (editor status shows `profile: dev` and mapping count > 0).
3. Open browser DevTools Console/Network as needed; open COS Console page/tab if used for device output.
4. Start serial log capture to file.
5. Record: firmware commit, frontend commit/build context, device, browser.

### A2. Compile-time / load-time Berry errors (serial + console)

1. Create or open a TypeScript file that compiles to Berry but triggers a Berry compile/load/runtime issue on device (examples: use a construct that generates valid-looking Berry but fails in pinned runtime context, or deliberately load a malformed `.be` on device if available). Prefer cases that produce Berry line references.
2. Compile in editor (dev) and note generated Berry line(s) for the suspected failing area.
3. Execute on device (e.g. `berry run ...`, app launch path, or equivalent command used in your workflow) and capture:
   - Serial output
   - WebSocket console transcript
   - Any frontend/editor error display
4. Repeat with at least 2-3 distinct error cases if possible:
   - Syntax/compile error reported by Berry runtime/compiler
   - Runtime exception/`raise` with line info
   - Undefined/global/member error with line info (if reported)

**Expected / evaluate:**
- Device/console output includes Berry line references in some format (e.g. `file.be:line:`, `line N`, stack-like traces). Record exact strings.
- **Editor manual annotator** ("Map Berry error/output to TypeScript"): pasting captured output annotates to correct TS source locations when mappings match.
- **Console auto-annotation** (shared compile metadata): recent transcript/pending device output is annotated automatically when it contains recognizable Berry line refs and mappings are available for the latest dev compile.
- Note any formats not recognized (needed for future hardening) and any false/incorrect mappings.

### A3. Runtime errors during app execution

1. Compile and run a simple TS app that throws/raises at a known source line (e.g. button press or timed action).
2. Trigger the error on device and capture serial + console output.
3. Compare mapped TS location to the original TS source line/construct.

### A4. Source-map fidelity spot checks

1. Use a TS sample with nested expressions, calls, property/element access, inline functions, and control flow.
2. Compile in dev and inspect generated Berry (`.be`) if available/exported.
3. Intentionally trigger or simulate line-referenced errors around different constructs and verify mapped locations are reasonable (best-effort; exact column precision may vary due to dedupe/source-start behavior).

### A5. Pass criteria / recording

- [ ] Real Berry/device error output samples captured (serial and console) with exact text.
- [ ] Manual annotator correctly maps recognizable line-referenced errors to TS for tested cases.
- [ ] Console auto-annotation behavior observed/documented (works where format/metadata match; limitations recorded).
- [ ] Any unrecognized formats or mismatches documented with raw samples for follow-up.
- [ ] No regressions/crashes in editor/console while compiling/annotating.

---

## Part B — Editor runtime memory diagnostics (frontend + device API)

**Goal:** Validate the confirmed runtime metrics endpoint and editor UI behavior.

**Confirmed API (current firmware):** `GET /api/heap` with HTTP Basic auth, `text/plain`, response body is free heap bytes as unsigned integer (e.g. `123456`).

### Steps

1. In editor, locate **Runtime memory diagnostics** panel.
2. With valid device auth and reachable device, click **Refresh**:
   - Expect status to show free heap in bytes (and formatted units), raw response, and last-updated time.
3. Enable **Auto-refresh (5s)** and run/load an app or perform actions; observe value updates or stability.
4. Test error handling:
   - Invalid/missing auth -> unauthorized state and sign-in prompt behavior as applicable.
   - Device unreachable/network issue -> unreachable/error state with message.
5. Compare sampled values with serial/shell diagnostics if available (`berry profile`, R5 `[R5]` logs, etc.) for sanity (not required to match exactly in timing).

### Pass criteria

- [ ] `/api/heap` values displayed correctly when authenticated and reachable.
- [ ] Auth and unreachable errors handled gracefully in UI.
- [ ] Manual refresh and auto-refresh (5s polling) work without UI hangs/leaks during a short observation window.
- [ ] Limitations noted (endpoint currently reports free heap only; richer metrics would require firmware/API changes).

---

## Part C — `BE_DEBUG_RUNTIME_INFO` / production firmware profile evaluation

**Goal:** Measure tradeoffs before any default firmware debug-config change. **Do not commit config changes unless explicitly decided from measured results.**

Reference: `docs/reports/2026-11-10-r14-prod-profile-debug-runtime-evaluation.md` and `components/berry_lang/src/berry_conf.h` (baseline currently `BE_DEBUG_RUNTIME_INFO 2`).

### C1. Baseline capture

1. Record commit hash, `components/berry_lang/src/berry_conf.h` values (`BE_DEBUG_RUNTIME_INFO`, `BE_DEBUG_SOURCE_FILE`, `BE_DEBUG_VAR_INFO`), and sdkconfig-relevant settings.
2. Build baseline:
   ```powershell
   idf.py fullclean   # optional but recommended for comparable size measurements
   idf.py build
   idf.py size
   idf.py size-components > artifacts/r14-baseline-size-components.txt
   ```
3. Flash and capture runtime baselines:
   - Boot + idle heap/watermarks (serial logs, `berry profile` if available).
   - Representative Berry error outputs (same cases as Part A if possible): syntax error, runtime exception/raise, stack trace quality/line info.
4. Save generated binaries/map/size outputs as needed for comparison.

### C2. Candidate measurement (only if/when evaluating a change)

1. **Temporarily** modify `BE_DEBUG_RUNTIME_INFO` in `components/berry_lang/src/berry_conf.h` on a local/build branch or working copy for measurement (record exact value). Do not commit unless adopting a decided policy.
2. Rebuild with identical environment:
   ```powershell
   idf.py build
   idf.py size
   idf.py size-components > artifacts/r14-candidate-size-components.txt
   ```
3. Flash and repeat the same runtime/error-output captures as baseline.
4. Restore baseline config after measurements unless intentionally keeping the change.

### C3. Compare and decide

Evaluate:
- [ ] Firmware/image and component size deltas (`idf.py size`, `berry_lang`/relevant components).
- [ ] Heap/watermark/largest-block differences at boot and under representative workloads.
- [ ] Berry runtime error/trace quality (line numbers, file/source info, stack usefulness).
- [ ] Impact on R14 automatic/manual mapping usefulness (if runtime omits line refs, auto-annotation cannot work).
- [ ] Any crashes, behavioral regressions, or build issues.

**Decision gate:** Keep baseline unless measurements show meaningful benefit with acceptable diagnostics/mapping impact. Prefer explicit build/profile variants over silent default changes.

---

## Reporting template (summary)

Create/update a results note (e.g. `docs/reports/2026-11-10-r14-hardware-results.md`) with:

- Date, tester, device, firmware commit, frontend build, browser
- Part A: cases run, captured error formats, annotator/console results, gaps
- Part B: `/api/heap` UI behavior, polling/auth/unreachable results
- Part C: baseline vs candidate measurements (if run), decision/recommendation
- Raw artifact paths and follow-up actions
