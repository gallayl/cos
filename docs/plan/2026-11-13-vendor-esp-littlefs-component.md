# Vendor `esp_littlefs` as a local COS component

**Created:** 2026-11-13  
**Status:** Implemented (local build/QEMU validation pending ESP-IDF v6.1 environment) — progress tracked in [docs/progress/2026-11-13-vendor-esp-littlefs.md](../progress/2026-11-13-vendor-esp-littlefs.md) 
**Scope:** Remove managed dependency `joltwallet/littlefs` and add `esp_littlefs` (wrapper + vendored upstream LittleFS) under `cos/components/` for use by `components/filesystem`.

## Summary

COS currently consumes `joltwallet/littlefs` (esp_littlefs v1.23.1) as an ESP-IDF managed component. `esp_littlefs` is a self-contained component that vendors upstream LittleFS (`src/littlefs/`) plus ESP VFS/partition glue, Kconfig, and image-generation hooks. Vendoring it into `cos/components/` is feasible and removes the external registry dependency while keeping existing behavior/API.

## Goals

- Eliminate `joltwallet/littlefs` from managed dependencies (`dependencies.lock`, component manifests) and resolve it from a local component in `cos/components/`.
- Preserve current filesystem behavior: mount/format/info/unregister via `esp_littlefs.h` used by `components/filesystem/src/flash.c`.
- Preserve build integration: `littlefs_create_partition_image(...)` in root `CMakeLists.txt` and `CONFIG_LITTLEFS_*` options.
- Keep change minimal and aligned with existing component conventions.

## Non-goals

- Reimplementing the ESP VFS/partition layer on top of raw `lfs.*` only.
- Upgrading LittleFS/esp_littlefs behavior beyond what is needed to vendor current version (unless required for IDF 6.1 compatibility).
- Refactoring filesystem APIs or mount/format logic unrelated to dependency wiring.

## Current state (baseline)

- **Managed dep:** `dependencies.lock` (target esp32, idf 6.1.0) direct dependency `joltwallet/littlefs` v1.23.1; `components/filesystem/idf_component.yml` requires `joltwallet/littlefs: "^1.20"`.
- **Consumer:** `components/filesystem/src/flash.c` includes `esp_littlefs.h` and uses `esp_vfs_littlefs_{register,unregister}`, `esp_littlefs_{mounted,info,format}` (`FLASH_MOUNT_POINT`, `FLASH_PARTITION_LABEL`).
- **Build hook:** `CMakeLists.txt:8` — `littlefs_create_partition_image(littlefs "${CMAKE_CURRENT_LIST_DIR}/storage/flash_data")` (provided by esp_littlefs `project_include.cmake`).
- **Consumer CMake:** `components/filesystem/CMakeLists.txt` currently `REQUIRES fatfs sdmmc driver` (no explicit LittleFS requirement declared).

## Source component (esp_littlefs) contents to vendor

- Upstream core (vendored): `src/littlefs/{lfs.c,lfs.h,lfs_util.c,lfs_util.h,LICENSE.md,...}`.
- ESP glue: `src/esp_littlefs.c`, `src/littlefs_esp_part.c`, `src/lfs_config.c`, `src/littlefs_bdl.c` (IDF≥6, non-8266), optional `src/littlefs_sdmmc.c`.
- Public API: `include/esp_littlefs.h`.
- Build/config: `CMakeLists.txt` (`-DLFS_CONFIG=lfs_config.h`, `PRIV_INCLUDE_DIRS src`, REQUIRES `esp_partition`/`esp_blockdev`/`sdmmc` as applicable, PRIV_REQUIRES `esptool_py spi_flash vfs`), `Kconfig`, `project_include.cmake`, `idf_component.yml`.
- Licenses: keep upstream LittleFS BSD-3-Clause (`src/littlefs/LICENSE.md`) and esp_littlefs license as applicable.

## Implementation plan

| Step | Change | Notes / acceptance |
|---:|---|---|
| 1 | Add vendored component under `cos/components/` (e.g. `components/esp_littlefs/` or `components/littlefs/`) | Copy full component incl. `src/littlefs/`, `Kconfig`, `project_include.cmake`. Choose component dir/name so IDF resolves it for consumers and for `littlefs_create_partition_image`. |
| 2 | Wire `components/filesystem` to local component | Add LittleFS component to `REQUIRES`/`PRIV_REQUIRES` in `components/filesystem/CMakeLists.txt` (match chosen component name). Remove managed dep from `components/filesystem/idf_component.yml` if no longer needed. |
| 3 | Remove managed dependency from project | Remove `joltwallet/littlefs` from `dependencies.lock` (`dependencies`, `direct_dependencies`) and ensure manifest/lock are consistent (regenerate via IDF tooling as appropriate). `managed_components/joltwallet__littlefs` should no longer be required. |
| 4 | Verify config/build hooks | `CONFIG_LITTLEFS_*` available; root `littlefs_create_partition_image(littlefs ...)` still resolves; compile flag `-DLFS_CONFIG=lfs_config.h` preserved. Target esp32 / IDF 6.1 keeps `esp_blockdev`/`littlefs_bdl.c` path valid. |
| 5 | Validate | Run relevant checks per `AGENTS.md` (at least `make build` in ESP-IDF v6.1 env; host/unit/integration as impacted). Record commands and outcomes. Confirm no behavior regressions in LittleFS mount/format flows (flash image generation intact). |

## Risks / decisions

- **Component naming:** must satisfy IDF component resolution for `components/filesystem` and for `project_include.cmake` function visibility; confirm chosen name before cleanup.
- **Lock/manifest regeneration:** `dependencies.lock` should be produced consistently by tooling rather than partial manual edits.
- **Maintenance ownership:** future esp_littlefs/upstream LittleFS fixes become COS-owned.
- **Behavior sensitivity:** COS has known sensitivity around mount-fail → format/remount (`components/filesystem/src/flash.c`, prior reports); keep runtime behavior identical.
- **SDMMC/Kconfig:** `CONFIG_LITTLEFS_SDMMC_SUPPORT` and other options remain available but unchanged unless explicitly configured.

## Validation checklist

- [ ] Managed dependency `joltwallet/littlefs` removed; build resolves local component only.
- [ ] `components/filesystem` links against vendored LittleFS component.
- [ ] `make build` succeeds (ESP-IDF v6.1) and `littlefs` partition image generation works.
- [ ] Relevant tests/checks run and recorded (per scope/availability).
- [ ] Hardware/QEMU-sensitive LittleFS behavior noted if not fully validated.

## Progress tracking

| ID | Task | Status | Evidence / notes |
|---:|---|---|---|
| V1 | Copy/vendor `esp_littlefs` (+ upstream `src/littlefs`) into `cos/components/` | ✅ Done | `components/esp_littlefs/` vendored from managed v1.23.1 (local source). |
| V2 | Update `components/filesystem` CMake/manifest to require local component | ✅ Done | `REQUIRES ... esp_littlefs`; `components/filesystem/idf_component.yml` cleared. |
| V3 | Remove `joltwallet/littlefs` from `dependencies.lock` / managed deps | ✅ Done | Removed from root and integration test locks; stale managed copies removed. |
| V4 | Confirm Kconfig + `project_include.cmake` / image generation integration | ✅ Done (static) | Component keeps `Kconfig`/`project_include.cmake`; integration tests using filesystem now include local component dir. Build hook not runtime-verified here. |
| V5 | Build and test validation, document outcomes | ✅/⚠️ Done (partial runtime) | `make format-check`, `make test`, `make build`, `make lint-db`, `make lint` pass; migrated Unity/integration-QEMU and hardware behavior not executed (see progress doc). |

## Follow-ups (analysis)

| ID | Task | Status | Notes |
|---:|---|---|---|
| F1 | Define/extend check coverage for vendored `esp_littlefs` | ✅ Done (partial) | **Structure:** component slimmed toward COS layout (non-COS scaffolding/payload removed; core wrapper + `src/littlefs` retained; Kconfig/project_include/licenses kept). **Format:** `format`/`format-check` include `components/esp_littlefs` (wrapper and `src/littlefs` core). **Lint-db:** `idf.py -B build/clang reconfigure` includes component in build graph/compile DB. **Lint:** clang-tidy (`make lint`) excludes vendored `esp_littlefs` to pass with project Werror rules (upstream/wrapper not rule-clean). **Test:** host `make test` unchanged/passing; `integration_test/test_esp_littlefs/` present but not executed. |
| F2 | Align tooling/library expectations for the vendored component | ✅ Done | Kept functional/attribution artifacts: `project_include.cmake` + `image-building-requirements.txt` (required for `littlefs_create_partition_image`, pinned `littlefs-python==0.15.0`), `Kconfig`, `LICENSE`/`src/littlefs/LICENSE.md`, and `idf_component.yml` as provenance metadata (origin v1.23.1, commit). Replaced upstream README with COS-specific vendoring/integration/maintenance notes to avoid misleading managed/submodule/PlatformIO instructions. Tooling alignment: covered by `format`/`format-check` and `lint-db`; excluded from `lint` (clang-tidy) by project policy; host `test` unchanged; Unity suite in `integration_test/test_esp_littlefs/`. |
| F3 | Simplify vendored component to COS needs (safe, behavior-preserving) | ⬜ Todo | Candidates: remove unused non-CMake scaffolding and upstream-only payload not needed at runtime (docs/tests/benches/runners/`bd/`/scripts/Makefiles/git metadata) while keeping required sources and `LICENSE`/attribution; drop/trim ESP8266 and IDF `<6.1` branches in CMake/code if irrelevant to COS target; reassess unused features/config (`CONFIG_LITTLEFS_SDMMC_SUPPORT` path vs COS SD handling, raw API, SPIFFS compat, multiversion, mmap, fcntl/open-dir extras, alt malloc/WDT options) and component `sdkconfig.defaults`. **Risk:** mount/format/image behavior and on-disk compatibility are sensitive (`components/filesystem/src/flash.c`)—validate with `make build`, image generation, and relevant integration/QEMU/hardware checks before/after pruning. |
