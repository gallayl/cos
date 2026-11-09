# TypeScript feature extension ideas

## Status

**Partially implemented** (as of 2026-11-08). The browser-based TS→Berry
compiler in `frontend/src/ide/berry/*` and SDK modules in
`frontend/src/cos-sdk/modules/*` cover several import/SDK items. Firmware-side
lazy module loading and several language mappings remain open.

## Progress summary

| Idea | Status | Notes / follow-ups |
|---|---|---|
| Type-only imports (`import type ...`, `import { type X }`) | Done | Compiler skips `ImportClause.phaseModifier == TypeKeyword` and type-only specifiers/elements (commits `1c945ae`–`de243ee`). |
| Custom syntax imports (relative) | Done | Relative `cos_import(...)` lowering with named/default/namespace support and export lowering exists (`statement-generator.ts`, tests in `ast-to-berry.test.ts`). |
| `@cos/*` / `@berry/*` module imports (compiler/SDK shape) | Partial | Compiler accepts `@cos/*` and `@berry/*` specifiers and lowers them via `cos_import`; SDK provides module declarations. **Missing:** firmware/runtime lazy/on-demand native module loading and removal of unnecessary eager globals/imports (see below). |
| On-demand Berry/native module loading (firmware) | Done (R6, hardware validated 2026-11-08) | R1 made `json` on-demand. R6 removed eager startup loading of `console`/`fs`/`net`/`ui`: `berry_runtime_init()` installs lightweight lazy proxy globals (dynamic module + Berry-closure `member` hook via internal `cos_lazy_member`). First bare-global attribute access (`ui.*`, `net.*`, etc.) loads the real native module from `be_modtab.c`, replaces the global, and returns the attribute; real module tables are not initialized at startup. `cos_import('@cos/*')` and Berry `import` load on demand and bypass proxies. TS-compiled shipped scripts/apps already use `cos_import` and are unaffected. Hardware validation (user, 2026-11-08): `tests/r6/lazy_modules.be` passed (`R6 OK`, including bare `ui.KEY_ENTER`/`console.font_size`), Breakout and Tap Dash launched/setup/closed cleanly. Initial proxy bug (`be_setmember` stack + native-hook path) was fixed and retested. |
| Harmonize Berry built-ins in TS SDK | Partial | `berry-builtins.d.ts` and per-module SDK files exist (`math`, `string`, `json`, `introspect`, `gc`, `global`, etc.). **Follow-up:** audit mappings/availability against actual Berry/COS runtime and avoid importing unused modules by default. |
| `.length` / string-size mapping review | Open / needs verification | Compiler currently lowers property `.length` to `.size()` (method). The edge case called out here should be validated against Berry semantics and fixed with tests if incorrect. Audit similar mappings (arrays, maps, bytes, strings). |
| `try`/`catch`/`throw` (compiler) | Done (R4, 2026-11-08) | Compiler lowers `throw`→`raise` and `try/catch`→`try/except .. [as e]/end` with frontend unit tests (`ast-to-berry` R4 cases). `finally` and destructuring catch bindings are diagnosed (Berry has no `finally`; catch maps to Berry's first exception value). User hardware validation passed (`berry run /sdcard/tests/trycatch.be` → expected output, `R4 OK`): `docs/reports/2026-11-08-r4-hardware-checklist.md`. |
| `async`/`await` | Not started | `async` functions, `await`, and `for await` are explicitly rejected. Requires runtime/concurrency design before syntax support; do not silently desugar. |

## Imports

### Type import

Syntax should be the standard `import type { MyCustomType } from './formSomewhere'`. Syntactic sugar only. As types are stripped during Berry conversion, this should not have runtime cost. Implementation should be simple, ROI is high.

### Custom syntax import

Syntax: `import myCustomStuff from './myCustomModule'` (file extensions can be tricky maybe). `myCustomStuff` (function, var, class, etc...) should be usable once imported.
Value: Code reusability, allows DRY implementation
Question: Compiling to berry? Relation with [Berry Imports](https://berry.readthedocs.io/en/latest/source/en/Chapter-8.html#how-to-package-a-module)?

### Berry built-in and native COS API imports

Current implementation: No need to import for e.g.: using the `ui` or the `net` module.
What we should try to investigate: 
 - Instead of always importing modules to Berry VM, we should do that on-demand. E.g.: A Berry script that don't use `net`, shouldn't load and allocate memory, buffers, etc... for `net` related functionality
 - Instead of using a global namespace, these should be also imported as a module. Syntax example: `import net from '@cos/net'`
 - We should also use these style of imports instead of the hard-coded ones that we have right now in `be_modtab.c`. I would prefer that nothing unneccessary should be imported by default as a hard coded default - remember, we are running low on RAM!
 - Module initialization should be lazy. See: [2026-11-06-memory-pressure-fixes - P2](./2026-11-06-memory-pressure-fixes.md) 
 - Once we use these style of imports, we should harmonize the Typescript SDK with the [built-in Berry modules](https://berry.readthedocs.io/en/latest/source/en/Chapter-7.html)


 ## Harmonize Berry built-ins

 - Berry provides a handful of [default libraries](https://berry.readthedocs.io/en/latest/source/en/Chapter-7.html). Once we fixed the module imports, we should update the Typescript SDK to support them from Typescript codes
 - We should also review our SDK defaults if there are some misalignments. 
    - An example: Determining string size.
        Current implementation: `myString.length` (TS) => `myString.size()` (Current Berry, invalid syntax), should be `size(myString)` (correct Berry syntax)
    - We should look for similar edge cases

## try / catch implementation

**Status (R4, 2026-11-08):** Done (compiler + manual hardware validation).

- Supported lowering:
  - `throw expr;` → `raise <compiled expr>`
  - `try { ... } catch (e) { ... }` → `try ... except .. as e ... end`
  - `try { ... } catch { ... }` → `try ... except .. ... end`
  - Catch parameter type annotations are erased.
- Berry semantics (pinned `65fce317...`, confirmed via `build/berry-source/tests/exceptions.be`, `class.be`, `compiler.be`, `division_by_zero.be`): `try`/`except`/`end`, `except ..` catches all, `except <type> as e[, m]` supported by Berry; `raise <value>` / `raise <value>, <message>` supported by Berry. TS catch currently binds Berry's first exception value only.
- Diagnosed (no silent miscompile): `finally` clauses (`try/finally`, `try/catch/finally`) — Berry has no `finally`; cleanup must be explicit. Destructuring catch bindings. `try` without `catch`.
- Tests: `frontend/src/ide/ast-to-berry.test.ts` R4 block. Manual/hardware guide: `docs/reports/2026-11-08-r4-hardware-checklist.md`.
 
 ## async / await syntax

 As we are started to use e.g.: networking, we should consider supporting async / await style code construction.

