# COS Simulator - Virtual CYD Development Environment

## 1. Overview

Introduce a browser-based COS Simulator that allows developers to execute and test COS applications without deploying them to a physical CYD device.

The simulator should provide a virtual CYD environment capable of executing actual Berry applications and displaying their UI in a browser.

The goal is to validate application logic, UI behavior and interactions before deploying applications to physical hardware.

The simulator should eventually integrate with the COS TypeScript SDK and TypeScript-to-Berry compiler.

## Implementation status (2026-11-08)

**Proposed / not implemented.** No Berry WebAssembly build, simulator worker,
platform-abstraction simulator backend, `SimulatorRenderer`, virtual CYD canvas,
or simulator UI integration was found in the repository during this review. The
frontend currently focuses on the TypeScript/Berry IDE, SDK declarations, and
compile/storage workflows (see [2026-11-03-ts-sdk.md](./2026-11-03-ts-sdk.md)).

### Follow-ups (prerequisites / sequencing)

- Pin and reproduce the exact Berry source revision/config used by firmware before
  building a WASM runtime.
- Define the minimal COS platform abstraction seam (display/input/clock/timers/
  filesystem/commands/logging) so ESP-IDF and simulator backends share
  application-facing APIs without compiling ESP-IDF to WASM.
- Start with a headless/diagnostic Worker + Berry VM lifecycle, then add a
  `Renderer` backend and canvas, then input/timers/filesystem shims.
- Treat network, camera, Bluetooth, and other hardware-backed features as explicit
  simulated/stubbed capabilities with clear "not validated on hardware" labeling.

### Core principles

1. Execute actual Berry code, not a JavaScript reimplementation of Berry.
2. Use the same Berry revision as the COS firmware.
3. Reuse existing COS application lifecycle, UI components and Berry API bindings wherever practical.
4. Separate hardware-specific functionality from application-facing APIs.
5. Keep the simulator completely independent of the ESP32 runtime.
6. Make simulation deterministic where practical.
7. Never pretend that simulated hardware functionality has been validated on real hardware.

**The simulator is an application-level virtual device, not a complete ESP32 emulator.**

It should simulate the COS execution environment, not ESP32 hardware, FreeRTOS or the electrical behavior of peripherals.

---

## 2. High-level architecture

```text
+----------------------------------------------------------+
|                  Browser / COS Studio                    |
|                                                          |
|  FuryStack Shades                                        |
|    +-- Monaco Editor                                     |
|    +-- COS TypeScript SDK                                |
|    +-- TypeScript-to-Berry Compiler                      |
|    +-- COS Simulator UI                                  |
|          +-- Virtual CYD Display                         |
|          +-- Console / Diagnostics                       |
|          +-- Virtual Input                               |
|          +-- Device Inspector                            |
|                                                          |
+-------------------------------+--------------------------+
                                |
                                v
+----------------------------------------------------------+
|                     Simulator Worker                     |
|                                                          |
|  Berry WebAssembly Runtime                               |
|    +-- Berry VM                                          |
|    +-- COS Native API Bindings                           |
|    +-- COS Application Lifecycle                         |
|    +-- COS UI Engine                                     |
|    +-- Simulator Platform Backend                        |
|          +-- Virtual Display                             |
|          +-- Touch / Keyboard                             |
|          +-- Timers / Clock                              |
|          +-- Virtual Filesystem                          |
|          +-- Command Registry                            |
|          +-- Simulated Network                           |
|                                                          |
+-------------------------------+--------------------------+
                                |
                                | Worker messages
                                |
                                v
+----------------------------------------------------------+
|                    Browser Main Thread                   |
|                                                          |
|  Canvas Renderer                                         |
|    +-- Display drawing commands                          |
|    +-- Text rendering                                    |
|    +-- Display scaling                                   |
|    +-- Rotation                                          |
|                                                          |
|  Input Dispatcher                                        |
|    +-- Mouse / Pointer                                   |
|    +-- Touch                                             |
|    +-- Keyboard                                          |
|                                                          |
|  Console and Diagnostics                                 |
|                                                          |
+----------------------------------------------------------+
```

### Main responsibilities

**Browser frontend:**

- Provide the virtual CYD interface.
- Display simulated graphics.
- Handle mouse, pointer, touch and keyboard interactions.
- Display console output and errors.
- Provide Run, Stop and Reset controls.
- Communicate with the simulator worker.

**Simulator worker:**

- Execute the Berry VM compiled to WebAssembly.
- Register COS-native Berry modules.
- Execute Berry application lifecycle operations.
- Simulate COS platform functionality.
- Process simulated input events.
- Send rendering commands and console events to the frontend.

**COS firmware:**

- No additional runtime responsibilities.
- Serve the frontend and WASM assets.
- Continue executing applications through the existing Berry VM.

The simulator must not consume ESP32 RAM or execute applications on the ESP32.

---

## 3. Berry WebAssembly runtime

### Technology

Use the actual Berry interpreter compiled to WebAssembly using Emscripten.

References:

- Berry project: https://github.com/berry-lang/berry
- Berry WebAssembly project: https://github.com/berry-lang/berry_web
- Berry online playground: https://berry-lang.github.io/try-online/
- Emscripten: https://emscripten.org/

The existing Berry WebAssembly project should be used as a reference and potential starting point.

Do not blindly depend on its precompiled binary.

### Version compatibility

The simulator must use the exact Berry source revision pinned by the COS firmware build.

Requirements:

- Use the same Berry upstream commit.
- Use compatible Berry configuration options.
- Keep relevant compiler and runtime feature flags aligned.
- Document unavoidable build differences between ESP-IDF and WebAssembly.
- Expose the Berry revision in simulator diagnostics.

The WASM build must be reproducible from the same Berry source revision used by the firmware.

### Runtime capabilities

The WASM runtime must expose a minimal C interface for:

- Initialize Berry VM.
- Register COS native modules.
- Load Berry source.
- Compile and execute Berry source.
- Execute the returned application class.
- Invoke application lifecycle methods.
- Close the active application.
- Run garbage collection.
- Retrieve Berry memory statistics.
- Retrieve compilation/runtime errors.
- Destroy and recreate the VM.

Use a dedicated Worker as the execution environment.

Do not expose browser DOM APIs directly to Berry applications.

### Worker isolation

The Berry VM must not execute on the browser main thread.

Requirements:

- Dedicated Web Worker.
- Communication through structured messages.
- Ability to terminate and recreate the worker.
- Execution watchdog.
- Configurable startup timeout.
- Recovery from application crashes or infinite loops.
- Independent runtime lifecycle.

A runaway Berry application must not freeze the editor.

The main thread must be able to terminate the worker even when WASM execution is stuck in an infinite loop.

The simulator should be able to recover by creating a fresh Worker and Berry VM.

### Emscripten configuration

Evaluate:

- Modularized WASM output.
- Worker-compatible runtime initialization.
- Explicit memory allocation limits.
- Optimized production output.
- Correct error reporting.
- No unnecessary browser environment dependencies.

Avoid pthreads and SharedArrayBuffer requirements in the initial implementation.

Pin the Emscripten toolchain version in the build environment.

---

## 4. Platform abstraction

This is the most important architectural requirement.

The existing COS application-facing Berry APIs should remain independent of ESP-IDF implementation details.

Introduce or extend a platform abstraction that allows the same COS functionality to run against two backends:

```text
          COS application APIs
                   |
                   v
            COS platform layer
                   |
          +--------+--------+
          |                 |
          v                 v
     ESP-IDF backend   Simulator backend
          |                 |
          v                 v
     Physical CYD      Browser / WASM
```

### Required platform services

Define platform interfaces for:

| Service       | ESP32 implementation         | Simulator implementation                 |
| ------------- | ---------------------------- | ---------------------------------------- |
| Display       | LovyanGFX / hardware display | Browser renderer                         |
| Clock         | ESP timer                    | Browser monotonic clock                  |
| Random values | ESP-IDF random               | Browser-side deterministic/random source |
| Logging       | ESP logging                  | Simulator console                        |
| Input         | Hardware touch / keyboard    | Pointer / keyboard events                |
| Timers        | ESP timers / UI scheduler    | Simulator scheduler                      |
| Filesystem    | COS VFS                      | Virtual filesystem                       |
| Commands      | COS command registry         | Simulated command registry               |
| Network       | ESP networking               | Simulated network                        |
| Brightness    | Hardware backlight           | Virtual display brightness               |

Keep the application-facing API signatures identical wherever practical.

The two platform backends may implement the same API differently.

### Important implementation rule

Do not attempt to compile the complete ESP-IDF firmware as WASM.

The simulator should have a lightweight native runtime that:

- Compiles Berry.
- Registers COS Berry modules.
- Reuses compatible COS UI/application code.
- Supplies simulated implementations of hardware-dependent services.

---

## 5. COS UI engine integration

The existing COS UI implementation should be reused as much as possible.

Relevant existing components:

- `components/ui/include/ui.h`
- `components/ui/src/ui.cpp`
- `components/berry_lang/src/berry_ui.cpp`

The current UI architecture already contains a `Renderer` abstraction and a hardware-oriented `DisplayRenderer`.

This should be used as the starting point for browser rendering.

### Renderer implementation

Introduce a simulator rendering backend.

Conceptually:

```cpp
class SimulatorRenderer : public ui::Renderer
{
public:
    void fill_rect(
        const Bounds &bounds,
        uint16_t color
    ) override;

    void draw_hline(
        int x,
        int y,
        int width,

```
