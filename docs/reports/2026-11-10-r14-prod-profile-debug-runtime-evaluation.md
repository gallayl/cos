# R14 - Production Profile / `BE_DEBUG_RUNTIME_INFO` Evaluation (Design & Measurement Plan)

**Date:** 2026-11-10  
**Status:** Evaluation/design only (no firmware config changes applied)  
**Scope:** R14 follow-up — dev/prod compiler profiles and production/debug-runtime options, including `BE_DEBUG_RUNTIME_INFO` implications.

## Current state (repo)

- **Firmware Berry config:** `components/berry_lang/src/berry_conf.h`
  - `BE_DEBUG_SOURCE_FILE 0`
  - `BE_DEBUG_RUNTIME_INFO 2`
  - `BE_DEBUG_VAR_INFO 0`
  - `BE_USE_DEBUG_HOOK 0`, `BE_USE_DEBUG_GC 0`, `BE_USE_DEBUG_STACK 0`, `BE_USE_DEBUG_MODULE 0`
- **Berry revision/config path:** pinned by `components/berry_lang/CMakeLists.txt` (Berry fetched/checked out to build source dir; generated with `-c components/berry_lang/src/berry_conf.h`).
- **Frontend compiler profiles (R14, 2026-11-10):** `dev` (default, debug metadata/mappings) and `prod` (same generated Berry semantics/output for supported inputs, reduced compiler debug metadata). Compiler profiles currently do **not** change firmware runtime debug flags.

## What `BE_DEBUG_RUNTIME_INFO` affects (to validate against pinned Berry)

`BE_DEBUG_RUNTIME_INFO` controls runtime debug information retained/emitted by Berry (line/source-related runtime diagnostics, traces, and related metadata behavior depend on Berry implementation/version). In this firmware config it is currently `2`.

**Potential benefits of reducing/disabling (if supported):**
- Smaller firmware/code or debug metadata footprint.
- Lower RAM/flash pressure for runtime debug structures (must be measured).
- Possibly reduced overhead in error/trace handling paths (must be measured).

**Risks/costs:**
- Less useful Berry runtime error messages/stack traces (harder device-side debugging).
- R14 TS↔Berry source mapping becomes more important, but runtime may no longer report precise Berry lines/locations needed for automatic mapping.
- Behavioral/performance impact is version/config-specific; must not be changed without evidence.

## Recommended evaluation approach (before any default change)

1. **Inventory exact semantics for pinned Berry revision** (`65fce317...`): confirm meaning of levels `0/1/2` in `berry.h`/debug implementation and how they interact with `BE_DEBUG_SOURCE_FILE`, `BE_DEBUG_VAR_INFO`, and COS exception reporting.
2. **Build matrix (firmware, ESP-IDF):** compare baseline (`BE_DEBUG_RUNTIME_INFO 2`) vs candidate prod setting(s) with identical source/config otherwise.
3. **Measure (before/after, record in report):**
   - `idf.py size` and relevant component/section sizes (`berry_lang`, total image).
   - Boot/runtime heap: free, largest block, minimum watermark (internal/default as applicable).
   - Berry script/app error diagnostics quality: compile/runtime error text, line reporting, stack traces for representative cases (syntax error, runtime exception, `berry run`/command-script failure).
   - Any performance impact in script compile/run hot paths if observable.
4. **Compatibility checks:**
   - Host/unit tests and Berry script validation (`scripts/validate-berry-scripts.ps1`) unaffected by firmware-only flag? (firmware runtime behavior only; verify generated `.be` unchanged).
   - Frontend R14 mapping UX: manual/automatic annotation can only help when runtime/device output includes Berry line references; reduced runtime info may limit automatic usefulness.
5. **Decision gate:** do **not** change default firmware config unless measurements show meaningful benefit and diagnostics/mapping workflows remain acceptable. Prefer exposing as an explicit build/profile choice (e.g. dev/prod sdkconfig/Kconfig or documented config variant) rather than silently changing production defaults.

## Suggested hardware/measurement checklist (pending)

- [ ] Record baseline build/flash: commit, `BE_DEBUG_RUNTIME_INFO`, `idf.py size`, `size-components`.
- [ ] Capture runtime error samples on device (baseline): syntax error, `raise`/exception, undefined method/global, stack trace if available.
- [ ] Rebuild with candidate setting; record same size/heap/error samples.
- [ ] Compare R14 console/editor annotation usefulness with captured outputs.
- [ ] Document decision: keep baseline, adopt candidate for prod profile only, or defer.

## R14 interaction

- **Compiler-side `prod` profile (implemented):** reduces compiler debug metadata/mappings, keeps generated Berry behavior unchanged. This is independent and safe as a frontend compile option.
- **Firmware `BE_DEBUG_RUNTIME_INFO` (not changed):** runtime/debug-trace behavior and error detail source. Any change requires firmware build + hardware/runtime validation and should be coordinated with R14 mapping/error UX expectations.

## Hardware validation artifacts (2026-11-10)

- Execution guide/checklist: `docs/reports/2026-11-10-r14-hardware-validation-guide.md` (Part C covers `BE_DEBUG_RUNTIME_INFO`/prod firmware evaluation).
- Helper script: `scripts/r14-hardware-validation.ps1` (`-FirmwareEval` collects context/config snapshot and can capture `idf.py` build/size outputs; does not modify config or flash).

## Outcome

User-reported hardware validation/evaluation completed and passed (2026-11-10, see `docs/reports/2026-11-10-r14-hardware-results.md`). No firmware config changes were adopted for R14 closure based on the reported outcome. Revisit only with measured evidence and explicit decision if future profiling warrants it.
