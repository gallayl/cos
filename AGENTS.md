# Project instructions

Shared repository instructions for all agent harnesses. Keep this file and nested `AGENTS.md` files as the source of truth for durable rules; reusable procedures live in `.agents/skills/` and are composed by thin tool-specific commands/agents when present.

## Scope and architecture

- **Firmware / host tests:** C/C++ in `components/`, `main/`, `test/`, and `integration_test/`.
- **Frontend / web IDE:** TypeScript in `frontend/` (see `frontend/AGENTS.md` for UI/framework conventions).
- **On-device scripts / SDK:** TypeScript sources in `storage/ts/`, compiled artifacts in `storage/flash_data/`, SDK declarations in `frontend/src/cos-sdk/`, and Berry/native bindings in `components/berry_lang/src/`.
- **Docs and plans:** `docs/` (see `docs/README.md` for structure/status meaning). `docs/plan/consolidated-improvement-plan.md` is the canonical tracker for planned/current work.

## Target and constraints

- **Target:** ESP32 (ESP-IDF 6.1), CYD board (ESP32-2432S028R).
- **Resource-constrained:** minimize RAM/flash usage at all times.

## C/C++ conventions (firmware, tests, integration tests)

- C++ with `extern "C"` public APIs for cross-component compatibility.
- No exceptions (`-fno-exceptions`), no RTTI (`-fno-rtti`).
- Prefer stack allocation over heap; avoid `std::string` in hot paths. Use fixed-size buffers where the upper bound is known.
- **ESP-IDF patterns:** structure code as ESP-IDF components under `components/`. Each component: `CMakeLists.txt`, `include/` (public headers), `src/` (implementation). Return `esp_err_t` from fallible functions; use `ESP_RETURN_ON_ERROR` macros. Log with `ESP_LOGx(TAG, ...)` using a `static const char *const TAG` per file. Use `esp_console` for CLI commands (not custom UART parsing). Use NVS for small config data, LittleFS/SPIFFS for larger files.
- **Naming (C/C++):** functions/variables `snake_case`; macros/constants `UPPER_CASE`; types/structs `snake_case_t` (C), `CamelCase` (C++ classes); component-prefixed APIs (e.g. `display_init()`, `calibration_load()`).
- **SOLID for embedded:** hide implementation behind C-compatible function APIs; isolate board-specific config in dedicated headers; one responsibility per component.

## TypeScript SDK and Berry bindings (invariant)

`frontend/src/cos-sdk/` declarations and native modules in `components/berry_lang/src/` are one script API. A change to either side updates the other in the same change.

| Script global | Declarations | Native registration |
| --- | --- | --- |
| `log`, `action` | `index.d.ts` | `berry_runtime.cpp` |
| `console` | `console.d.ts` | `berry_runtime.cpp` |
| `fs` | `filesystem.d.ts` | `berry_runtime.cpp` |
| `net` | `network.d.ts` | `berry_runtime.cpp` |
| `ui` | `ui.d.ts` | `berry_ui.cpp` |

- Use the same Berry name, argument order, and return value on both sides. Names stay `snake_case`.
- Document declarations with TSDoc comments so the editor can show them on hover.
- Declare only functions the device can run, and register only functions the SDK exposes.
- When a script-facing API is added/changed, add and compile a device example (see `.agents/skills/add-device-script-example/SKILL.md`).
- `language.d.ts` is the TypeScript subset the Berry compiler accepts; keep it aligned with `frontend/src/ide/berry/` (not a native module).

## Testing

- **L1 — Unit tests** (`test/`): host-based, CMake + Unity, mock hardware via fakes.
- **L2 — Integration tests** (`integration_test/`): ESP-IDF apps built for ESP32 and run in QEMU via `pytest-embedded-qemu`; exercise real NVS, LittleFS, FreeRTOS, and `esp_console` on emulated hardware.
- **L3 — Real hardware:** peripherals without QEMU support (display, WiFi, ADC, LEDC) stay at L1 or L3.
- Target ~100% coverage on logic modules; hardware-init modules are 0% by design.
- Prefer automated, repeatable checks; flag changes that lack adequate test coverage.

## Documentation (Doxygen)

- All public API functions, structs, enums, and typedefs in `include/` headers must have Doxygen comments.
- Use `/** */` style. Functions need at least `@brief`; add `@param`, `@return` when non-obvious. Struct members use inline `/**< description */` comments.
- Internal/static helpers in `src/` files do not require Doxygen docs.
- Run `make docs-check` to validate; it must pass with zero warnings.

## Build & tooling

- Always use the `Makefile` targets for building, testing, formatting, and linting. Never invoke `cmake`, `idf.py`, `clang-format`, `clang-tidy`, `doxygen`, or `ctest` directly; use the Makefile wrappers instead.
- Available targets: `build`, `flash`, `monitor`, `clean`, `fullclean`, `format`, `format-check`, `lint`, `test`, `integration-test`, `menuconfig`, `docs`, `docs-check`.
- The Makefile auto-detects ESP-IDF tools via `IDF_TOOLS_PATH` and `IDF_PYTHON_ENV_PATH`.

## Code quality

- All code must pass `clang-format`, `clang-tidy`, and `doxygen` checks (see project root configs).
- Zero compiler warnings (`-Werror`).
- Keep changes consistent with surrounding code and conventions; avoid unnecessary/obvious comments, keep existing meaningful comments.

## Definition of done (verify before declaring a change complete)

Be proactive: after making changes, run the relevant checks yourself without waiting to be asked, fix failures, and re-run until they pass. Do not declare work complete based on code edits alone. The only exception is when validation requires physical hardware/on-device interaction—if so, run everything you can locally/automatically, then clearly state what remains and ask for hardware validation.

Report exactly which commands were run and their outcomes. Do not rely on CI to catch issues. If a check cannot be run or fails due to missing tools/environment, explain the concrete blocker and help the user resolve it (what to install, where/how to configure it, and how to rerun the check). Never claim a check passed unless it was executed.

### Default checks (most changes)

- `make format-check` — formatting must match CI (`.github/workflows/format-check.yml`).
- `make test` — host-based unit tests must pass (`.github/workflows/test.yml`).

If formatting fails, run `make format`, then re-run `make format-check`.

### Additional checks by scope

| Scope | Checks | Notes |
| --- | --- | --- |
| Frontend / `storage/ts` / `storage/flash_data` changes | From `frontend/`: `yarn format:check`, `yarn lint`, `yarn test`, `yarn build` | Matches `.github/workflows/frontend.yml` (Node 24.x, Yarn). Run all four proactively after frontend-related changes. `yarn build` runs TypeScript checking (`tsc`) and Vite build. If deps are missing or stale, run `yarn install --frozen-lockfile` in `frontend/` (requires Node.js 24.x and Yarn installed, with network/access to registry as needed). |
| Firmware / build / lint-covered C-C++ changes | `make build`, and `make lint` when feasible | CI (`.github/workflows/build-and-lint.yml`) builds with ESP-IDF v6.1 and runs clang-tidy against a clang compile DB (`build/clang/compile_commands.json`). These require an ESP-IDF v6.1 environment sourced (`IDF_PATH`, tools; see `README.md`). Do not claim build/lint passed if the environment is unavailable. |
| Documentation / Doxygen comments | `make docs-check` | Matches `docs-check` CI workflow. |
| Integration tests / QEMU flows | `make integration-test-build` and/or `make integration-test` (or `make integration-test-run` after build) | See `README.md` and `.github/workflows/integration-test.yml`; requires ESP-IDF v6.1, QEMU tool, and pinned deps in `integration_test/requirements.txt`. Run when changes affect integration-tested behavior. |

### Reporting

- Summarize results per command (pass/fail) and mention any fixes applied.
- Run checks proactively by default; do not wait for the user to ask to rebuild, run tests, lint, or format-check unless the step requires hardware validation.
- Do not rely on CI. If you could not run or complete a required check (missing ESP-IDF/QEMU/tooling, Node.js/Yarn, wrong environment, etc.), explain the specific cause and give actionable steps to fix it: what to install, required versions, where to install/set it (e.g. `IDF_PATH`, `IDF_TOOLS_PATH`, `PATH`, sourcing `export.sh`/IDF environment; for frontend: install Node.js 24.x and Yarn, run `yarn install --frozen-lockfile` in `frontend/`), and the exact command to rerun.
- Prefer the Makefile targets above for firmware/host checks and the frontend `yarn` scripts in `frontend/` for frontend checks so local verification stays aligned with CI.

## Agentic workflow

- Shared, reusable procedures live in `.agents/skills/` (layered: primary entry procedures compose granular skills). Harness-specific commands/agents/subagents, when used, should be thin adapters that reference these shared instructions/skills.
- Keep changes consistent with the structure, naming, style, and patterns of the surrounding code. Treat unfamiliar files or changes as potential user work and investigate before deleting or overwriting them.
