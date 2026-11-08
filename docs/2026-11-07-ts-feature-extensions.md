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
| On-demand Berry/native module loading (firmware) | Not started | `berry_runtime_init()` still eagerly installs globals for `console`, `fs`, `net`, `ui`, `json`; `json` is also loaded at startup despite `berry_runtime_ensure_json()` infrastructure. Align with P2/lazy-loading goal. |
| Harmonize Berry built-ins in TS SDK | Partial | `berry-builtins.d.ts` and per-module SDK files exist (`math`, `string`, `json`, `introspect`, `gc`, `global`, etc.). **Follow-up:** audit mappings/availability against actual Berry/COS runtime and avoid importing unused modules by default. |
| `.length` / string-size mapping review | Open / needs verification | Compiler currently lowers property `.length` to `.size()` (method). The edge case called out here should be validated against Berry semantics and fixed with tests if incorrect. Audit similar mappings (arrays, maps, bytes, strings). |
| `try`/`catch` | Not started | `TryStatement`/`ThrowStatement` are explicitly rejected by the compiler. Map to Berry `try`/`except`/`raise` semantics with clear diagnostics and tests. |
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

 Users should be able to use a standard, Typesciprt / Javascript-style Try / Catch block. Berry VM should already have a `try` / `except` syntax that we should use here. However it's not used right now, we should use it as a standard error handling mechanism
 
 ## async / await syntax

 As we are started to use e.g.: networking, we should consider supporting async / await style code construction.

