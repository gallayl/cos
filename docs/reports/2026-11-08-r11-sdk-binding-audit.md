# R11 — SDK single-source-of-truth / binding audit (2026-11-08)

**Status:** pragmatic audit/validation implemented (no hardware validation required).

## Scope

R11 required SDK declarations, module exports, and docs to match actual Berry/COS bindings, with automated validation preferred to prevent drift. Covered: `ui`, `console`, `fs`, `net`, `log`, `action`, Berry built-in module semantics, and `@cos/*` / `@berry/*` module resolution.

## Binding inventory (firmware sources)

| Surface | Firmware source | Notes |
|---|---|---|
| `ui` module | `components/berry_lang/src/berry_ui.cpp` | 25 functions + 16 constants. `ui.init` is registered but treated as internal by the SDK (runtime hook; not intended for app scripts). |
| `console` module | `components/berry_lang/src/berry_runtime.cpp` (`console_attrs`) | 12 functions. |
| `fs` module | `components/berry_lang/src/berry_runtime.cpp` (`fs_attrs`) | 9 functions. |
| `net` module | `components/berry_lang/src/berry_network.cpp` (`net_attrs`) | 8 functions. |
| Public globals | `components/berry_lang/src/berry_runtime.cpp` (`be_regfunc`) | `log`, `action` declared in SDK. `cos_import`/`cos_module` are compiler/runtime helpers; `cos_lazy_member` is internal (R6). |

Berry built-in modules exposed through `@cos/*`/`@berry/*` and present in SDK declarations: `string`, `json`, `math`, `global`, `gc`, `introspect`. Their detailed stdlib API is owned by Berry/upstream docs; R11 validates declaration presence and resolver aliases.

## Drift fixes

- **`@berry/*` editor resolution:** `frontend/src/ide/cos-sdk-manifest.ts` now registers `@berry/*` specifiers alongside `@cos/*` for all SDK module declarations. Runtime `cos_import()` and the TS compiler/storage tsconfig already accepted `@berry/*`; Monaco path mappings previously exposed only `@cos/*`.
- **`json` availability note:** `frontend/src/cos-sdk/json.d.ts` documents R1/R6 lazy loading: prefer explicit `@cos/json`/`import json`; the global is installed by runtime JSON paths (e.g. `fs.read_json`/`write_json`, `net.fetch` JSON parsing).

## Automated validation

Frontend tests (Vitest):

- `frontend/src/ide/cos-binding-inventory.ts` — explicit expected binding inventory for COS native modules/globals and module specifier semantics.
- `frontend/src/ide/cos-binding-audit.ts` — pure helpers that extract `be_native_module_function`/`be_native_module_int` registrations and `be_regfunc` globals from C++ source text and assert SDK declarations cover inventory names.
- `frontend/src/ide/cos-binding-audit.test.ts` — cross-checks inventory vs firmware C++ registration tables, checks public globals are registered, checks global SDK namespaces/files declare expected names (excluding internal hooks like `ui.init`), checks manifest registers `@cos/*` and `@berry/*` specifiers/wrappers, and keeps fs/net legacy aliases.
- `frontend/src/ide/cos-sdk-manifest.test.ts` updated for `@berry/*` alias coverage.

Validation run (2026-11-08):

```text
cd frontend
npx vitest run src/ide/cos-binding-audit.test.ts src/ide/cos-sdk-manifest.test.ts
# 2 files, 10 tests passed
npx tsc --noEmit
# passed
```

## Follow-ups (optional)

- Move inventory closer to firmware (generated artifact or shared checked-in schema) if C++/SDK churn increases.
- Expand Berry stdlib member-level checks for `string`/`math`/`json`/`gc`/`introspect`/`global` only if/when those APIs are treated as part of the stable COS SDK contract.
- If a binding/SDK semantic mismatch is found that depends on runtime behavior, add a guided hardware validation item (none required by this audit).