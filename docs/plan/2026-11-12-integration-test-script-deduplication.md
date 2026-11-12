# Integration Test Script Deduplication

**Created:** 2026-11-12  
**Status:** Proposed / not started  
**Scope:** Integration tests that seed flash with hardcoded Berry scripts (initial focus: `integration_test/test_shell_fs`)  
**Related:** `docs/plan/2026-11-12-berry-command-script-migrations.md`, `storage/flash_data/`, `AGENTS.md` (DoD/validation)

## Problem

Some integration tests embed Berry script source as C string literals and write them into the test filesystem at runtime. For example, `integration_test/test_shell_fs/main/test_shell_fs.c` defines scripts such as `s_ping_script`, `s_autoexec_script`, and `s_berry_*` in `seed_test_files()` and writes them to `/flash/*.be`.

This creates duplication and maintenance risk:

- **Multiple sources of truth:** the same conceptual command/script (e.g. `ping`, `autoexec`) may exist as a hardcoded test string and as a canonical script under `storage/flash_data/` (e.g. `storage/flash_data/scripts/ping.be`, `storage/flash_data/autoexec.be`).
- **Drift risk:** fixes or behavior changes made in one place are easy to miss in the other, so tests may stop representing real/stock behavior or may encode outdated contracts.
- **Poor reviewability:** large inline script blobs in C reduce readability and make script diffs harder to review than `.be` files.
- **Copy-paste across tests:** as more integration tests need similar scripts, inline duplication tends to spread unless a shared mechanism exists.

## Goals

- Reduce or eliminate duplicated script content between integration tests and canonical/test-fixture sources.
- Make it clear which scripts are **canonical/stock** vs **test-only fixtures**.
- Keep integration tests deterministic and isolated (tests should still control the exact flash contents/behavior they rely on).
- Preserve existing test contracts and validation coverage unless an intentional change is documented.

## Non-goals

- Changing shell/Berry command behavior as part of deduplication (unless required to align with an explicitly chosen canonical script and covered by tests).
- Reworking all integration tests at once; start with the highest-duplication cases (e.g. `test_shell_fs`).
- Making tests depend on a developer machine’s mutable filesystem at runtime in a way that breaks reproducible CI/QEMU builds.

## Current observations (initial example)

- `integration_test/test_shell_fs/main/test_shell_fs.c` seeds inline scripts including `ping.be` and `autoexec.be`.
- `storage/flash_data/scripts/ping.be` and `storage/flash_data/autoexec.be` exist as canonical/stock scripts but differ in style/content from the test’s inline versions (for example, stock scripts may use module/class/entrypoint conventions such as `cos_import`/`__cos_entry` that the simplified test scripts do not). This means “deduplicate” is not always a byte-for-byte replacement without considering runtime/contract compatibility.

## Possible approaches

### 1. Embed or generate C data from source `.be` files at build time (recommended for reuse)

- Use the integration test build (CMake/ESP-IDF) to read canonical or fixture `.be` files and produce consumable artifacts for the firmware test app:
  - `file(READ ...)` / `configure_file` to generate headers or C sources containing string literals/byte arrays, or
  - ESP-IDF embedding mechanisms for text/binary blobs, or
  - generated include files compiled into the test binary.
- `seed_test_files()` (or equivalent) writes the embedded/generated contents to the target VFS paths.
- **Pros:** single authored script file, good diff/review, reproducible in CI, no runtime dependency on host files.  
- **Cons:** build plumbing/generation step; need conventions for where source scripts live.

### 2. Package fixture/canonical scripts into a flash image/partition for the test

- Build or prepare a LittleFS/flash image containing the required scripts and mount/use it for the integration test instead of (or in addition to) programmatic seeding.
- **Pros:** closest to real flash contents; can reuse existing image tooling if present.  
- **Cons:** depends on existing test/partition/image workflow; may reduce per-test flexibility unless layered/overlaid.

### 3. Introduce shared integration-test fixtures

- For scripts that are **test-only** (not intended to match stock behavior), store them as `.be` files in a shared fixture location (e.g. `integration_test/common/fixtures/` or per-suite `fixtures/`) and embed/generate from there.
- Keep per-test fixtures only when behavior is unique to that test.
- **Pros:** removes copy-paste across tests while preserving intentional test-specific behavior.  
- **Cons:** requires deciding fixture ownership/naming and build integration.

### 4. Seed directly from canonical `storage/flash_data` where contracts match

- When a test needs stock behavior, source the script from `storage/flash_data/...` rather than an inline copy.
- Requires the test runtime to be compatible with the canonical script’s dependencies and conventions (imports, resolver/entrypoint expectations, available modules/APIs).
- **Pros:** strongest guarantee that tests exercise real scripts.  
- **Cons:** not always possible without aligning script style or test environment; may couple tests to broader stock layout changes.

## Recommended direction

1. **Classify each hardcoded script** in scope as either:
   - **Canonical/stock-equivalent:** should come from `storage/flash_data/...` (or a single canonical source), or
   - **Test-only fixture:** should live as an explicit `.be` fixture file (shared if reused).
2. **Prefer build-time sourcing/embedding** (Approach 1/3) so scripts are authored as `.be` files but tests remain reproducible and self-contained.
3. **Avoid blind replacement** of inline scripts with stock scripts when formats/contracts differ (e.g. `ping.be` style differences noted above). Either:
   - make the test consume the canonical script and update the test to the canonical contract/environment, or
   - keep a clearly named fixture if the test intentionally needs a simpler/specific behavior.
4. **Pilot on `integration_test/test_shell_fs`**, then apply the same pattern to other integration tests with similar inline scripts.

## Risks and considerations

- **Behavior/contract drift during deduplication:** replacing a simplified inline script with a canonical one can change args parsing, output, imports, or entrypoint behavior; tests and expectations may need coordinated updates.
- **Runtime dependencies:** canonical scripts may rely on `cos_import`, module globals, path/resolver behavior, or flash layout not present in minimal integration-test environments.
- **Build complexity:** generation/embedding must work in ESP-IDF/integration-test builds and CI environments (including Windows developer setups per project tooling).
- **Determinism:** tests must continue to seed known contents; avoid accidental dependence on unrelated files from `storage/flash_data`.
- **Scope creep:** keep the change focused on script sourcing/deduplication; do not mix with unrelated command migrations except where necessary.

## Suggested implementation plan (incremental)

1. Inventory inline scripts in `integration_test/test_shell_fs` and identify canonical equivalents or fixture status.
2. Introduce a small, repeatable mechanism for the test to consume `.be` files (generated header/embedded blob/fixture copy) at build time.
3. Migrate one low-risk script first (e.g. a clearly test-only helper) to validate the mechanism.
4. Migrate remaining scripts, resolving canonical-vs-fixture decisions per script (especially `ping.be`/`autoexec.be`) and updating test expectations only where intentional/required.
5. Run relevant integration/firmware checks per `AGENTS.md`, record outcomes, and document the chosen convention for future tests.

## Validation checklist (when implemented)

- [ ] `integration_test/test_shell_fs` passes with no unintended behavior changes.
- [ ] Seeded flash contents remain deterministic and match the intended source (canonical or fixture).
- [ ] Any intentional contract changes (if aligning to canonical scripts) are reflected in tests/docs.
- [ ] Relevant checks run and reported per `AGENTS.md` (at minimum integration test build/run for affected suite; format/check/build as applicable to touched components).
- [ ] Convention documented so future integration tests avoid reintroducing inline script duplication.
