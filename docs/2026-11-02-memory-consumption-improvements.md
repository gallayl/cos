# COS Memory Optimization - Phase 1

## Low-risk improvements before TypeScript support

### Status

**Proposed / largely not started** (as of 2026-11-08). Related but separate
network/Berry memory work landed in `e0bf05b` (see
[2026-11-06-memory-pressure-fixes.md](./2026-11-06-memory-pressure-fixes.md));
it does not satisfy the Phase 1 acceptance criteria below.

### Progress

- [ ] **Instrumentation baseline:** no `be_gc_memcount()`/watermark/heap-block
      reporting integrated into the lifecycle checkpoints described here was
      found in `berry_runtime`/shell diagnostics during this review.
- [ ] **Release source buffers immediately:** `read_virtual_file()` /
      `execute_app_source()` ownership still needs verification/change against
      the pinned Berry implementation.
- [ ] **Lifecycle cleanup audit:** teardown, callback/timer/UI cleanup, and
      post-GC leak checks need focused regression tests (repeated start/stop).
- [ ] **GC tuning experiment:** `be_gc_setsteprate()` benchmarks (200/150/125)
      not recorded; production default unchanged.
- [x] **Preserve existing optimizations:** precompiled objects / constrained
      stack settings remain in place unless measurements justify changes.
- [ ] **Host-side Berry validation:** no dedicated CI target found that compiles
      all existing `.be` applications as a compiler regression gate.

### Follow-ups

- Add a repeatable measurement harness for the seven lifecycle checkpoints,
  including `be_gc_memcount()` and ESP heap watermarks/largest block.
- Make source-buffer lifetime explicit around `be_loadbuffer()` and add failure
  tests (compile error, missing file, VM pressure).
- Add restart-loop tests for representative apps to detect leaks/fragmentation
  before changing GC defaults.

### Goals

Reduce unnecessary RAM consumption, startup memory peaks and memory leaks without changing the existing Berry VM architecture or breaking existing applications.

### 1. Instrumentation and baseline measurements

Improve the existing `mem` command and application lifecycle logging.

Track:

- Total free ESP32 heap.
- Free internal heap.
- Largest available internal heap block.
- Minimum free heap (low watermark).
- Berry VM allocated memory (`be_gc_memcount`).
- Memory usage before and after application initialization.
- Memory usage after application teardown and garbage collection.

Capture measurements at:

1. Berry VM initialization.
2. Application source loading.
3. Berry compilation.
4. Application construction.
5. `setup()` completion.
6. Application teardown.
7. Garbage collection completion.

Add a repeatable application restart test to identify memory leaks and fragmentation.

### 2. Release source buffers immediately

Review `read_virtual_file()` and `execute_app_source()` in `berry_runtime.cpp`.

Currently, the complete source file is allocated in RAM and remains allocated during application execution.

Change the ownership/lifetime of the source buffer:

- Keep the buffer alive until `be_loadbuffer()` finishes.
- Release it immediately after compilation.
- Handle compilation failures correctly.
- Avoid double-free or ownership ambiguity.

Verify the behavior against the pinned Berry implementation.

Expected benefit: lower peak RAM consumption during application startup, proportional to source size.

### 3. Audit application lifecycle cleanup

Review the existing application cleanup implementation.

Verify that:

- `teardown()` is called.
- Application instances are released.
- Application class references are released.
- Application-specific global references are removed.
- UI callbacks are unregistered.
- Timers are stopped.
- UI resources are destroyed.
- Garbage collection runs after cleanup.

Add regression tests that repeatedly start and stop different applications.

Pay particular attention to Berry globals, because global references can prevent otherwise unused objects from being collected.

Avoid introducing new global references without an explicit cleanup strategy.

### 4. Experiment with Berry GC tuning

Current Berry configuration uses a GC step rate of 200.

Benchmark alternative values:

- 200: current baseline.
- 150: moderately more aggressive collection.
- 125: more aggressive collection.

Use `be_gc_setsteprate()`.

Measure:

- Peak memory consumption.
- Minimum available heap.
- GC execution frequency.
- Application responsiveness.
- Frame-rate impact, where applicable.

Do not change the production default until measurements demonstrate a worthwhile improvement.

### 5. Preserve existing optimizations

Keep the existing precompiled-object configuration:

`BE_USE_PRECOMPILED_OBJECT = 1`

Keep the currently constrained Berry stack settings unless measurements demonstrate that they need adjustment.

Avoid speculative changes to stack allocation limits.

### 6. Host-side Berry validation

Introduce a host-side Berry compiler validation workflow.

Add a CI target that:

- Compiles all existing `.be` applications.
- Detects syntax and compilation errors.
- Validates scripts before deployment.
- Provides a repeatable basis for future compiler regression tests.

### Acceptance criteria

- Existing Berry applications continue working unchanged.
- Repeated application switching does not cause progressive memory loss.
- Source buffers are released immediately after compilation.
- Application cleanup is verified.
- GC changes are benchmarked against the existing configuration.
- Memory measurements are available before and after each application lifecycle stage.

### Explicit non-goals

- No changes to the Berry VM architecture.
- No new scripting language.
- No JavaScript runtime.
- No solidification changes.
- No changes to the existing application format.
- No PSRAM support.
