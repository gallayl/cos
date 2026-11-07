# Typescript feature extension ideas

## Imports

### Type import

Syntax should be the standard `import type { MyCustomType } from './formSomewhere'`. Syntactic sugar only. As types are stripped during Berry conversation, this should not have runtime cost. Implementation should be simple, ROI is high.

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

