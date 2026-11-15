# COS Memory Optimization - Phase 2

## Optimizations enabled by the TypeScript SDK and compiler

### Status

**Mostly proposed / in progress** (as of 2026-11-10). The browser TS→Berry
compiler and COS SDK foundations exist (see
[2026-11-03-ts-sdk.md](./2026-11-03-ts-sdk.md)). **R14 slice 1 (2026-11-10):**
compiler `dev`/`prod` profiles, debug metadata foundation, statement-level
source-map utilities, and incremental editor compile-summary diagnostics were
implemented in the frontend compiler/editor with tests (see
[docs/reports/2026-11-10-r14-compiler-profiles-debug-metadata.md](../reports/2026-11-10-r14-compiler-profiles-debug-metadata.md)).
Remaining R14 work (finer/runtime-error mapping, `BE_DEBUG_RUNTIME_INFO`
evaluation with measurements, runtime-memory editor diagnostics), bytecode/
solidification work, and native UI allocation changes are not implemented as
specified. R13 memory-conscious compiler rules and entrypoint/resource-contract
standardization were delivered separately (2026-11-09).

### Progress snapshot

- [x] **Memory-conscious compiler rules (R13, 2026-11-09):** always-on entrypoint
      diagnostics/rules implemented for app structure/state/lifecycle contract
      (`cos/default-class-entrypoint`, `cos/entrypoint-contract`,
      `cos/no-top-level-exec`, `cos/no-module-state`,
      `cos/no-top-level-function`, `cos/top-level-class`, etc.). Broader
      configurable hot-callback/allocation rules remain follow-up.
- [~] **Resource lifecycle / `Disposable` / `CosApplication`:** `CosApplication` /
      `CosUiApplication` SDK/compiler contract standardized in R13; guaranteed
      native cleanup registration beyond conventions remains follow-up.
- [~] **Compiler profiles (dev/prod), debug metadata, source maps (R14, 2026-11-10
      slice 1):** `dev`/`prod` profiles, debug metadata, statement-level
      source-map foundation/utilities, and incremental editor compile-summary
      diagnostics implemented; finer/runtime-error mapping and firmware debug/
      `BE_DEBUG_RUNTIME_INFO` evaluation pending.
- [ ] **Persistent Berry bytecode (`.bec`) workflow/loader:** not found.
- [ ] **Native UI allocation benchmarking/policy changes (`berry_ui.cpp`/`ui.cpp`):**
      not evidenced as part of this phase.
- [ ] **Berry solidification evaluation for firmware-bundled code:** not found.
- [ ] **Memory diagnostics in the frontend/editor:** not found.
- [ ] **Application memory metadata/policies:** not found.

### Follow-ups

- Start with low-risk compiler diagnostics that prevent common leaks (timers/
  callbacks without cleanup patterns, accidental globals) and document them.
- Add measurement-driven work only after Phase 1 instrumentation and 2026-11-06
  profiling baselines exist.
- Treat `.bec`, solidification, and allocation-policy changes as separate,
  benchmarked experiments with rollback paths.

### Goals

Use the TypeScript compiler to prevent avoidable application memory problems, improve deployment efficiency and explore more substantial Berry runtime optimizations.

### 1. Enforce memory-conscious application patterns

Use the TypeScript compiler and custom diagnostics to enforce predictable application structure.

Recommended rules:

- Require a single default-exported application class.
- Discourage application-level mutable globals.
- Keep application state inside the application instance.
- Disallow unsupported top-level executable statements.
- Require explicit lifecycle methods where resource cleanup is necessary.
- Discourage uncontrolled object and array creation in frequently executed callbacks.
- Reject unsupported language features instead of silently generating potentially incompatible Berry code.

These rules should be configurable and documented.

### 2. Introduce resource lifecycle management

Extend the COS SDK with a consistent resource-management model.

Potential API:

```typescript
interface Disposable {
  dispose(): void;
}

interface CosApplication {
  setup(content: unknown, width: number, height: number): void;

  teardown(): void;
}
```

Explore resource-management helpers for:

- Timers.
- UI event callbacks.
- Registered listeners.
- UI handles.
- Other resources requiring explicit cleanup.

The application lifecycle should guarantee that registered resources are released during teardown.

This may require additions to the native Berry API.

### 3. Add production compiler configuration

Introduce compiler profiles.

**Development profile:**

- Readable generated Berry code.
- Runtime line information.
- Detailed diagnostics.
- Easier debugging.

**Production profile:**

- Optimized generated Berry code.
- Optional removal of runtime line information.
- Reduced debug metadata.
- Smaller deployment artifacts.

Investigate disabling:

`BE_DEBUG_RUNTIME_INFO`

for production firmware/application builds.

Because this reduces runtime diagnostic information, investigate TypeScript source maps so Berry runtime errors can eventually be mapped back to original TypeScript source locations.

### 4. Explore persistent Berry bytecode (.bec)

Introduce an optional bytecode deployment format.

Potential workflow:

```text
TypeScript source
       |
       v
TypeScript compiler
       |
       v
Berry source
       |
       v
Berry bytecode compilation
       |
       v
.bec deployment artifact
       |
       v
Existing Berry VM
```

Benefits:

- No source parsing at runtime.
- No runtime source-to-bytecode compilation.
- Faster application startup.
- Potentially lower startup memory peaks.
- Smaller deployment artifacts in some cases.

Important limitations:

- Bytecode still requires runtime allocations.
- It does not substantially reduce steady-state application RAM.
- Bytecode compatibility must be tracked against the Berry version.
- The COS VFS loader must support the precompiled format.

Keep `.ts` as the authoritative application source.

Consider `.bec` an optional generated deployment artifact rather than a replacement for source files.

### 5. Investigate native UI allocation improvements

Review native UI memory management in:

- `berry_ui.cpp`
- `ui.cpp`

Investigate:

- Vector allocation retention.
- String allocation patterns.
- UI container lifecycle.
- Callback registry allocation.
- Timer registry allocation.
- Action queue capacity.
- Application teardown allocation release.

`std::vector::clear()` does not release allocated capacity.

Evaluate selectively releasing container capacity after application teardown.

Compare:

- Retaining allocations for faster subsequent application launches.
- Releasing allocations to reduce persistent heap usage.

Avoid blindly shrinking every container, as repeated allocations can increase fragmentation.

### 6. Investigate Berry Solidification

Evaluate Berry Solidification for applications that are bundled with COS firmware.

Solidification converts compiled Berry structures into static firmware objects stored in flash.

Potential benefits:

- Reduced dynamic allocation.
- Lower runtime RAM usage.
- Faster initialization of embedded code.
- Lower heap pressure for built-in applications.

Important limitations:

- Solidified structures are read-only.
- Dynamic user applications cannot simply be solidified at runtime.
- Closures with captured upvalues have important restrictions.
- Global-variable handling has additional constraints.
- String constants have limitations.

Prioritize built-in COS functionality rather than user-deployed applications.

Do not make solidification a requirement for ordinary TypeScript application deployment.

### 7. Introduce memory diagnostics in the editor

Expose runtime memory information through the COS development frontend.

Display:

- Current free heap.
- Internal heap availability.
- Largest free memory block.
- Minimum free heap.
- Berry VM allocated memory.
- Application startup memory consumption.
- Memory usage after application teardown and GC.

Consider tracking memory trends across repeated application runs.

Provide warnings when memory usage increases unexpectedly or available contiguous heap becomes critically low.

### 8. Explore application memory policies

Introduce optional application metadata for runtime monitoring and memory diagnostics.

Example:

```typescript
interface CosApplicationMetadata {
  memory?: {
    maxBytes?: number;
    gcStrategy?: "balanced" | "aggressive";
    allowLargeBuffers?: boolean;
  };
}
```

Potential future functionality:

- Application memory usage warnings.
- Configurable GC behavior.
- Runtime memory budgets.
- Detection of excessive allocations.
- Clear reporting when insufficient memory prevents application startup.

Important: `maxBytes` should initially be treated as a monitoring target, not a guaranteed hard limit. Enforcing per-application allocation limits requires additional allocator/runtime work.

### Acceptance criteria

- TypeScript applications follow predictable lifecycle and resource-management rules.
- Unsupported memory-sensitive language constructs produce meaningful compiler diagnostics.
- Development and production compiler profiles work correctly.
- Bytecode deployment is optional and backward compatible.
- Native UI allocations are benchmarked before changing allocation policies.
- Solidification is evaluated independently from dynamic application deployment.
- Memory diagnostics are available in the COS frontend.
- Existing Berry applications remain compatible.

### Explicit non-goals

- Replacing Berry with JavaScript.
- Requiring MicroQuickJS.
- Requiring solidification for user applications.
- Promising arbitrary npm package compatibility.
- Assuming bytecode eliminates runtime heap allocations.
- Introducing per-application memory isolation without a separate architectural evaluation.

### Long-term objective

Make COS applications memory-conscious by design, using compiler diagnostics, predictable resource lifecycles, optimized deployment artifacts and better runtime observability.
