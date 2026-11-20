# R20e Image Support Feasibility Spike — Raw RGB565 VFS → Display (ESP32 CYD)

- **Date:** 2026-11-20
- **Item:** [`docs/plan/done/r20-image-support-feasibility-minimal.md`](../plan/done/r20-image-support-feasibility-minimal.md) (`r20-image-support-feasibility-minimal`, R20e)
- **Type:** Feasibility spike + written recommendation (docs/report only; no firmware/SDK implementation in this change)
- **Target hardware context:** ESP32 (ESP-IDF 6.1), CYD ESP32-2432S028R, ILI9341 240×320 RGB565 (`CYD_PANEL_WIDTH=240`, `CYD_PANEL_HEIGHT=320`, `components/display/src/cyd_board_config.h`), LovyanGFX 1.3.0 (`components/display/idf_component.yml`)
- **Hardware validation:** **Not performed in this spike.** Estimates and code inspection only; assumptions stated below. Any minimal-slice implementation requires on-device (CYD) validation with heap/largest-block/watermark measurements around load+draw (R2/R5 style).

## 1. Recommendation (go/no-go)

**Go (conditional, minimal v1 only).** A tightly bounded **raw/preconverted RGB565** image path loaded from VFS (`/flash`, `/sdcard`) and drawn via a firmware/UI-layer primitive with thin Berry/Cos SDK bindings is **feasible** on the current architecture, provided v1 enforces strict caps, avoids whole-image heap allocation by using streamed/chunked reads and small fixed line buffers, and is validated on CYD before claiming success.

**Do not go (v1):** general-purpose decode of PNG/JPEG/BMP subsets or unbounded/large image loading. These add decode CPU time, flash/RAM footprint, licensing/maintenance cost, and heap/fragmentation risk disproportionate to current constraints.

## 2. Candidate approaches considered

| Approach | Feasibility | Memory/CPU | Complexity/licensing | Notes |
|---|---:|---:|---:|---|
| **A. Raw/preconverted RGB565 file, streamed line-by-line from VFS to display** | **Good (recommended v1 target)** | Low RAM if streamed (fixed line/chunk buffer, e.g. `w*2` or bounded chunk); no decode CPU | Very low; no third-party decoder | Data is display-native (16-bit). Requires agreed file format/header or explicit dimensions. Fits LovyanGFX push/write primitives (wrapper needed). |
| **B. Raw RGB565 loaded into one contiguous heap buffer, then pushed once** | Risky at/near caps | `w*h*2` contiguous bytes (e.g. 96×96 = 18,432 B) | Very low | **Not recommended** as default: R2/R5 evidence shows default-heap largest contiguous blocks can fall well below 18 KB under fragmentation/load (e.g. 4–9 KB observed in stressed states), making contiguous allocation fragile. Acceptable only if implementation proves/uses safe allocation strategy and small caps with measured headroom. |
| **C. BMP (constrained subset, e.g. 24-bit/16-bit uncompressed, no palette tricks beyond minimal subset) parsed + converted on device** | Possible but weaker v1 | Needs row decode/convert buffer and file parsing; extra CPU | Moderate (parser code), low licensing if in-house | Useful for tooling convenience, but adds parsing/format edge cases vs raw. Could be a follow-up if raw path is validated. |
| **D. PNG decode (e.g. lodepng/miniz/tinf-based)** | Poor for minimal v1 | High peak RAM/stack/CPU for decode buffers/state; zlib/inflate cost | Higher code/flash; license review per library | Unlikely to fit tight RAM/fragmentation budget safely at useful sizes without careful streaming decoder and significant integration effort. |
| **E. JPEG decode (e.g. TJpgDec/tjpgd/ROM decoder paths)** | Poor for minimal v1 | Decode buffers/MCU blocks + CPU; ROM decoder availability/constraints vary | Moderate–high integration; license per library | Better than PNG for photos but still non-trivial; not aligned with "smallest useful path" for icons/sprites on CYD. |

**Conclusion:** prioritize **A (raw/preconverted RGB565, streamed)**. Evaluate B only with strict caps/measured contiguous headroom; treat C/D/E as comparison/follow-up notes, not v1 commitments.

## 3. Current architecture evidence (code inspection)

- **Display pipeline (LovyanGFX):** `components/display/src/display.cpp` owns a `CydDisplay : lgfx::LGFX_Device` (ILI9341 + SPI + PWM backlight + XPT2046 touch) and exposes C wrappers: `display_fill_screen/rect/hline/vline`, `display_draw_text/char`, `display_draw_text_row` (row sprite), `display_start_write/end_write/wait`, touch/rotation/brightness. **No bitmap/image push API is exposed today.** LovyanGFX 1.3.0 is pinned as a git dependency (`components/display/idf_component.yml`), and the library natively supports pixel-buffer operations (e.g. push/write image APIs) that could be wrapped behind a thin, bounded C API.
- **UI framework:** `components/ui/src/ui.cpp` runs a FreeRTOS UI task (20 ms poll/redraw cadence, 4096-byte stack in `ui_task`), draws via `Renderer` abstraction (`fill_rect/draw_hline/draw_vline/draw_text`) and composites widget tree into a display write transaction. No image element/primitive exists in `ui::Renderer` or widgets (`components/ui/include/ui.h`).
- **Berry/Cos SDK bindings:** `components/berry_lang/src/berry_ui.cpp` registers `ui` natives (widget handles, direct drawing `fill_screen/fill_rect/draw_text`, touch/key/timer, etc.). `frontend/src/cos-sdk/ui.d.ts` declares the same surface (kept in sync per project invariant). No image load/draw function is exposed.
- **VFS/filesystem:** `components/filesystem/include/filesystem.h` provides `vfs_resolve_path`, `vfs_stat`, `vfs_read_file` (whole-file into caller buffer), and `vfs_read_file_stream(path, callback, context)` for bounded synchronous chunked reads (callback data valid only during call). Flash LittleFS and SD card mounts support `/flash` and `/sdcard` virtual paths when available/mounted (`sdcard_is_mounted()`, `vfs_resolve_path`/`vfs_is_virtual_path` behavior).
- **Display/transaction model:** existing batched writes use `display_start_write/end_write`; row rendering uses an internal 16-bit sprite sized to display width × font row height (`display.cpp`). An image primitive should follow existing SPI/write patterns and avoid holding the bus longer than necessary; streaming small chunks/lines inside a bounded transaction strategy is compatible.

## 4. Memory, performance, and stack notes (estimated)

> **Assumptions (not measured):** ESP32 CYD-class device with tight/fragmented default heap as seen in R2/R5/R8 reports; ILI9341 SPI write throughput dominated by SPI bus; raw RGB565 source file stored preconverted; no additional PSRAM assumed for v1. Validate all numbers on hardware in a follow-up implementation.

### 4.1 Pixel/data sizes (RGB565 = 2 bytes/pixel)

| Image size | Raw pixel bytes | Full contiguous buffer | One-line buffer | 4-line buffer |
|---:|---:|---:|---:|---:|
| 32×32 | 2,048 B (~2.0 KiB) | 2,048 B | 64 B | 256 B |
| 64×64 | 8,192 B (8.0 KiB) | 8,192 B | 128 B | 512 B |
| 96×96 | 18,432 B (18.0 KiB) | 18,432 B | 192 B | 768 B |
| 96×64 | 12,288 B (12.0 KiB) | 12,288 B | 192 B | 768 B |

- **Tentative cap hypothesis (item scope):** ≤ 96×96 px and/or ≤ ~20 KiB RGB565. A 96×96 raw image fits the byte budget but a **single contiguous 18 KB allocation is not reliably safe** under observed fragmentation (R2/R5 recorded largest default blocks dropping to ~3–9 KB in stressed cases; healthy states can be larger but vary). **Streaming with a fixed line/chunk buffer (≤ ~1 KiB, preferably 192–512 B) is the recommended memory model** and should be treated as a design constraint for v1.
- **Stack:** v1 should not place image line buffers on deep call stacks. Use a small fixed stack-local buffer only if proven safe, or a bounded static/scratch buffer owned by the implementation path with clear lifetime; avoid large stack frames in UI/Berry call paths. UI task stack is currently 4096 bytes (`components/ui/src/ui.cpp`); image work should not run inside constrained callbacks with large locals.
- **Flash/storage:** raw preconverted assets consume exactly pixel bytes (+ optional tiny header) in LittleFS/SD; no decoder code/RO data added.

### 4.2 Performance (rough estimate)

- Transfer cost scales with pixels: `w*h*2` bytes over SPI. At display write-clock-class throughput, small images (e.g. 32×32 to 96×96) should be visually quick enough for static icons/splash/reference use; exact timing must be measured on CYD (SPI clock/config, transaction overhead, FS read latency differ between `/flash` and `/sdcard`).
- VFS chunked streaming (`vfs_read_file_stream`) reads in bounded chunks; per-chunk callback overhead is acceptable if chunk size is aligned to one/few scanlines. SD card latency/variability should be measured separately from flash.

### 4.3 Compatibility with LovyanGFX/display pipeline

- **Compatible in principle:** LovyanGFX supports writing 16-bit pixel data; COS already uses 16-bit color (`uint16_t` RGB565) throughout display/UI APIs. A thin `display_*` wrapper can push scanlines/rects using LovyanGFX primitives inside `start_write/end_write` batching.
- **Integration point:** prefer a firmware-level C API in `components/display` (and optionally a UI convenience wrapper only if it fits the existing renderer/task model without adding risk). Keep memory ownership in firmware/C path; Berry binding passes path/coords/dimensions/caps only.
- **Byte order / pixel format:** must be specified and validated (native RGB565 matching panel/LGFX swap config). Preconversion tooling/asset pipeline should produce the expected byte order for CYD; v1 spec must state this explicitly.

## 5. Licensing and complexity

- **Raw RGB565 path:** no new third-party runtime decoder dependency; minimal added code (path validation, bounded reads, small loops, wrapper calls). Lowest maintenance and audit surface.
- **LovyanGFX:** already vendored/pinned (BSD-like permissive license; verify exact terms in dependency checkout when implementing). No license change expected for using existing capabilities.
- **BMP/PNG/JPEG decoders:** each introduces additional license obligations and maintenance; only justified with clear need and measured budget. Not recommended for minimal v1.

## 6. VFS source rules and operational considerations

- **Supported sources (target):** virtual paths resolved by `vfs_resolve_path` under `/flash` and `/sdcard` (when SD mounted/available). Implementation must reject unsupported/non-virtual paths and missing/unmounted targets with clear errors (not silent failure).
- **Read model:** prefer `vfs_read_file_stream` for streaming to avoid whole-file heap buffer. If a header is used, read/validate header first (small fixed read) then stream pixel data.
- **Error/failure modes to define in v1:** file not found, path not allowed/unresolved, SD unavailable, stat/read errors, invalid/unsupported header or dimensions, dimensions exceed caps, file size mismatch/truncation, image partially out of screen bounds (define clip vs reject), draw/start-write failures if applicable.
- **Concurrency/lifecycle:** display access must respect existing UI/task/write coordination; v1 should define whether calls are allowed only from safe contexts (e.g. UI/Berry execution context) and be single image operation per call.

## 7. Risks and constraints

- **Heap fragmentation/contiguous allocation:** primary risk for naive full-buffer designs. Mitigation: streamed fixed buffers, strict caps, reject oversize early, measure largest block/watermarks.
- **Stack pressure:** avoid large local buffers in UI/Berry paths. Mitigation: small fixed scratch in firmware-owned storage with explicit lifetime.
- **Performance variability:** SD reads and SPI contention can affect latency; acceptable for static/small images but must be measured.
- **Format ambiguity:** raw RGB565 needs explicit width/height/byte-order contract. Mitigation: require API-provided dimensions and/or tiny validated header in v1 spec.
- **No hardware evidence in this spike:** estimates only; on-device validation is mandatory before claiming the implemented slice is reliable.

## 8. Minimal v1 spec (proposed, **if go**)

This spec is a concrete proposal for a follow-up implementation item/PR (not implemented here).

### 8.1 Scope
- Add a **minimal image draw primitive** for **raw/preconverted RGB565** images read from VFS and rendered to the display.
- **Architecture:** firmware/display-layer primitive (and Berry/Cos SDK binding) with memory ownership and bounds/cap enforcement in firmware. No UI widget/image element required in v1.
- **Single operation per call**, synchronous/bounded work only (no background decode, no unbounded streaming).

### 8.2 Format
- **Primary format:** raw RGB565 pixel data in the byte order expected by the CYD/LovyanGFX display path (exact endian/byte-swap rule to be documented and validated on hardware; asset conversion must match).
- **Container/header (recommended):** minimal fixed header to reduce caller error, e.g. magic + version + width(u16) + height(u16) (+ optional flags/reserved) followed by `width*height*2` bytes. Alternative acceptable for first slice: raw payload with explicit `(path, x, y, w, h)` API arguments and strict file-size check `== w*h*2` (or header size + payload). Choose one and document clearly; reject ambiguity.
- **Not in v1:** PNG/JPEG/BMP decode, alpha blending, scaling/rotation, animation, palettes.

### 8.3 Caps and limits (validate/adjust on hardware)
- **Max dimensions:** 96×96 px (initial hard cap). May allow non-square within same pixel/byte budget (e.g. 128×64 only if explicitly added and measured) — default proposal is **width ≤ 96, height ≤ 96, width*height*2 ≤ 18432**.
- **Max file/payload size:** ≤ 18,432 bytes payload (+ tiny header if used, still ≤ ~20 KiB total) consistent with tentative hypothesis.
- **Memory model:** streaming/chunked read + fixed line (or small multi-line) buffer; **no mandatory contiguous full-image allocation**. Reject requests that cannot be satisfied within bounded buffers/caps.
- **VFS sources:** `/flash` and `/sdcard` only (when mounted/available). Absolute virtual paths resolved/validated via existing VFS helpers.
- **Placement:** draw at `(x, y)` in screen coordinates. V1 behavior must be explicit: either **reject** images partially/fully outside display bounds or **clip** safely; recommend **reject out-of-bounds** for simplest safe v1 unless clipping is trivial and tested.

### 8.4 Proposed API shape (high level)

> Exact names/arg order to be finalized during implementation while keeping Berry/Cos SDK parity (`components/berry_lang/src/berry_ui.cpp` ↔ `frontend/src/cos-sdk/ui.d.ts`) and `snake_case` for Berry/native conventions.

- **Firmware/display primitive (C):** e.g. `esp_err_t display_draw_image_rgb565(const char *virtual_path, int x, int y, int w, int h)` or a header-aware variant. Validates path/caps/bounds, streams from VFS, writes pixels via LovyanGFX in bounded chunks, returns specific `esp_err_t` on failure.
- **Berry binding:** `ui.draw_image(...)` or `ui.image(...)` (name TBD) calling the firmware primitive; raises/returns errors consistently with existing UI/native patterns.
- **TypeScript SDK:** matching declaration in `frontend/src/cos-sdk/ui.d.ts` with TSDoc (path rules, caps, format/byte-order, error behavior, threading/context notes if any).

### 8.5 Error and failure modes
- Invalid args / null path / negative coords or dimensions → `ESP_ERR_INVALID_ARG` (or Berry exception per binding convention).
- Path not virtual/not allowed/unresolved, SD not mounted → `ESP_ERR_INVALID_ARG` / `ESP_ERR_NOT_FOUND` / `ESP_ERR_INVALID_STATE` as appropriate (document mapping).
- File missing / stat-read failure / stream read failure / truncated data → propagated `esp_err_t` with no partial silent success.
- Exceeds caps (dimensions/payload) or format/header invalid → `ESP_ERR_INVALID_SIZE` or `ESP_ERR_INVALID_ARG` (choose consistently and document).
- Out-of-bounds placement (if rejected in v1) → `ESP_ERR_INVALID_ARG`.

### 8.6 Validation and example plan (follow-up implementation)
- **Automated (when code changes):** `make format-check`, `make test`; `make build` and `make lint` when feasible in ESP-IDF env; frontend checks if `frontend/src/cos-sdk/ui.d.ts` changes; `make docs-check` if Doxygen/public headers touched (per `AGENTS.md`).
- **Hardware/manual (required):** on CYD load+draw known-good and negative-case images from `/flash` and `/sdcard` (when available): success path, missing file, oversize/cap reject, invalid format/header, unmounted SD case. Measure and record free heap, largest block, and min-watermark before/during/after operations (R2/R5 style), plus visual correctness and approximate draw time.
- **Reference example:** add a tiny preconverted RGB565 asset and a sample Berry/TS app or script demonstrating `ui.draw_image(...)` from `/flash` (and `/sdcard` if feasible) with documented conversion steps.

### 8.7 Follow-up implementation breakdown (suggested)
1. Add bounded `display` C API + VFS-streamed RGB565 draw (caps/header/validation) with Doxygen in public header.
2. Wire Berry `ui` binding and TS `ui.d.ts` declaration in the same change (SDK↔binding invariant).
3. Add host/unit coverage where feasible for argument/cap validation helpers; add on-device validation guide/script and record evidence.
4. Add sample asset + example and update docs.

## 9. Automated checks guidance for future code changes

Per `AGENTS.md`: run default checks (`make format-check`, `make test`) and scope-appropriate checks (`make build`, `make lint` when feasible, frontend checks for `frontend/`/SDK/TS changes, `compile-storage` verification if storage TS/examples change, `make docs-check` for Doxygen/public header docs). For this spike item (docs/report/plan updates only), no firmware/frontend/runtime code was modified.

## 10. Open questions / blockers recorded

- **No blocking unknowns found in spike scope.** Remaining implementation-time decisions (exact header vs explicit-dimensions API, exact error-code mapping, clip-vs-reject bounds, byte-order convention for assets) are bounded and must be finalized in the follow-up spec/implementation with hardware validation. If implementation reveals a hard blocker (e.g. insufficient reliable heap headroom even with streaming at proposed caps), record it and do not claim success.