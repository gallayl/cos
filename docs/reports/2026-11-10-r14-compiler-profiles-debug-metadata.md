# R14 - Compiler Profiles, Debug Metadata, Source Maps, Editor Diagnostics (Slice 1)

**Date:** 2026-11-10  
**Status:** In progress (slice 1 implemented; frontend/compiler-only, no firmware/runtime changes)  
**Scope:** `docs/plan/consolidated-improvement-plan.md` R14 — Compiler profiles, debug metadata, source maps, editor diagnostics (M9, P2)

## Goals for this slice

- Add explicit **compiler profiles** (`dev` / `prod`) with behavior-preserving defaults.
- Provide a **debug metadata foundation** (profile, source file name, notes, statement-level mappings).
- Add an initial **source-map strategy/utilities** for mapping generated Berry locations back to TypeScript.
- Incrementally surface profile/metadata in the **editor diagnostics/status UX**.
- Keep existing generated Berry behavior unchanged for supported inputs.

Non-goals for this slice: Web Worker compilation, firmware `BE_DEBUG_RUNTIME_INFO` changes, runtime Berry error translation pipeline, full expression-level source maps, editor memory/runtime diagnostics UI.

## Implementation

### Compiler API

- `frontend/src/ide/berry/diagnostics.ts`
  - Added `BerryCompileProfile` (`'dev' | 'prod'`).
  - Added `BerrySourceLocation`, `BerrySourceMapEntry`, `BerryCompileMetadata`.
  - Extended `BerryGenerationResult` with optional `metadata?: BerryCompileMetadata`.
- `frontend/src/ide/berry/code-generator.ts`
  - `BerryCompileOptions.profile?: BerryCompileProfile` added (default: `dev`).
  - `GenerationSession` records statement-level mappings in `dev` profile (`generated` line/column from writer position, `source` line/column from TS node, `kind` hint). Duplicate mappings at identical generated/source positions are deduplicated.
  - `prod` profile keeps generated Berry identical for supported inputs but emits **reduced debug metadata** (`mappings: []`, profile/notes retained).
  - Parse failures return metadata with empty mappings and a parse-failure note.
  - Metadata notes document that Berry runtime line/stack debug details depend on firmware build options (for example `BE_DEBUG_RUNTIME_INFO`); compiler metadata is the TS↔Berry mapping foundation.
- `frontend/src/ide/berry/context.ts` / generators
  - Added `GenerationContext.noteMapping(node, columnOffset?)` for construct/expression debug mapping hooks with optional 0-based generated-column offset from statement indent.
  - Class declarations, constructors/init emission points, methods, and function declarations record explicit mappings in addition to statement-level emission mappings.
  - Control-structure emission points (`if`/`elif`/`else`, `while`, C-style `for` condition/init, `for-of` iterable, `try`/`catch`, `switch` discriminant/arms/default) record additional mappings to improve Berry-line-to-TS localization.
  - Key expression mappings recorded at statement emission points and in expression lowering for calls, `new`, property/element access, and binary expressions (including assignments), plus variable initializers / return / throw / expression statements. Known statement prefixes (`var <name> = `, `return `, `raise `, `if `/`elif `/`while `, `for <name>: `, `self.<field> = `, switch arm conditions) provide **generated-column offsets** for mapped expressions/conditions.
  - **1a span-aware expression emitter prototype (`frontend/src/ide/berry/expression-generator.ts`)**: expressions are lowered to `EmittedExpression { text, precedence, spans }` with per-subexpression offsets (identifiers/literals, unary/binary operands, calls/`new` args/callees, property/element access targets/indexes, arrays/objects/templates). `emitExpressionMapped` / `emitGroupedMapped` / `noteExpressionSpans` propagate spans to statement/class emission sites so nested subexpressions map to generated columns relative to statement prefixes. Generated Berry output remains unchanged for supported inputs. Default parameter initializers propagate span mappings via `parameterList`/`beginFunction` offsets. Concise inline arrow/function expressions now return internal spans (parameter defaults and concise return expression) so nested mappings are recorded relative to the inline expression within the outer statement.
  - **Module organization (hardening/refactor, behavior-preserving):** extracted import/export declaration emission from `frontend/src/ide/berry/statement-generator.ts` into `frontend/src/ide/berry/module-declarations.ts`.
  - **Mapping/lookup hardening:** `findSourceLocation` now selects the latest mapping at or before the target generated position (correct column handling, including unordered same-line span mappings). `annotateBerryErrorText` hardened for additional synthetic Berry/runtime line-reference formats with idempotent annotation.
  - **Line-aware spans + block-body inline function/capture remapping:** expression spans are line/column-aware (supporting multiline emitted expression fragments). `capture()` collects fragment-relative mappings and block-body inline function emission remaps them to final generated positions (including nested block-body inline cases), instead of emitting fragment-relative coordinates. Global dedupe by identical generated/source positions reduces duplicate mappings.
  - Limitations: mappings can include more entries in dev; dedupe collapses different nodes that share the same generated position and same TS source start. Some embedded/multiline formatting edge cases and mapping-volume/source-specificity tradeoffs may still benefit from further refinement; real device/runtime output-format coverage remains best-effort without hardware/runtime samples.
- `frontend/src/ide/berry/writer.ts`
  - Exposed `nextLine` / `nextColumn` (1-based) to support mapping collection.
- `frontend/src/ide/ast-to-berry.ts`
  - Re-exported R14 types/utilities (`BerryCompileProfile`, `BerryCompileMetadata`, `BerrySourceLocation`, `BerrySourceMapEntry`, `findSourceLocation`, `mapBerryLineToSource`, `formatMappedSource`, `annotateBerryErrorText`).

### Source-map foundation

- `frontend/src/ide/berry/source-map.ts`
  - `findSourceLocation(metadata, generated)` — best-effort lookup (latest mapping at/before generated line/column).
  - `mapBerryLineToSource(metadata, berryLine)` — convenience Berry-line → TS-location mapping.
  - `formatMappedSource(metadata, berryLine)` — formatted `file:line:column` / `line:column` label for future diagnostics/runtime-error UX.
  - `annotateBerryErrorText(text, metadata)` — best-effort annotation of Berry/runtime/compiler error text (e.g. `:line:` / `line N` patterns) with mapped TypeScript locations; foundation for future runtime-error UX when device/runtime output is surfaced in the frontend.

Current mapping granularity is **statement + construct + control-flow + key-expression level** with a **span-aware expression emitter prototype (1a)**. Prefix-based column offsets and propagated expression spans provide generated-column positions for nested subexpressions in key statement forms, but precision/hardening is still incomplete for all emission contexts and node shapes (and same-source-start/generated-position mappings are deduplicated by design). Further refinement may be needed for full expression-span fidelity and real runtime-output mapping.

### Editor diagnostics (incremental)

- `frontend/src/editor/editor-document.ts`
  - Added `formatCompileSummary(result)` showing compile status, profile, and mapping count.
- `frontend/src/editor/editor-app.tsx`
  - Compile and live diagnostics use explicit `DEFAULT_COMPILE_PROFILE = 'dev'`.
  - Successful compile status now includes profile/mapping summary (e.g. `Compiled · profile: dev · mappings: N`); failed compile status includes error count/profile/mapping summary.
  - Stores latest compile metadata (including live TypeScript diagnostics compiles) for mapping-assisted tooling; metadata is cleared when replacing/resetting the document.
  - Existing Monaco marker flow for Berry diagnostics remains unchanged (TS locations reported by compiler diagnostics).
- `frontend/src/editor/berry-error-annotator.tsx` (new)
  - TypeScript editor panel: paste Berry/runtime/compiler error text and annotate line references to TS locations using latest dev compile metadata via `annotateBerryErrorText`.
  - Disabled with guidance when no mappings are available (e.g. before compile or prod/no-mapping metadata). Provides immediate UX value without requiring device/runtime output integration.

### Tests

- `frontend/src/ide/berry/source-map.test.ts`: dev mapping lookup, construct/control-flow/class/method mappings, key-expression mappings (calls/access/assignments), generated-column offset mappings for prefixed statements/expressions, span-aware nested subexpression/default-parameter/concise-inline-function column mappings, nested call/element/new span coverage, unordered same-line lookup hardening, remapped block-body inline function (including nested multiline capture) mappings, synthetic Berry/runtime error-text annotation formats, prod reduced metadata, parse-failure metadata.
- `frontend/src/ide/ast-to-berry.test.ts`: `astToBerry R14 profiles and debug metadata` (default dev metadata/mappings, dev/prod identical code for supported input with prod mappings omitted, parse-failure metadata).
- `frontend/src/editor/editor-document.test.ts`: `formatCompileSummary` coverage.

## Validation

Frontend-only validation:

- `cd frontend && npx vitest run src/ide/ast-to-berry.test.ts src/ide/berry/source-map.test.ts src/editor/editor-document.test.ts src/ide/cos-binding-audit.test.ts src/ide/cos-sdk-manifest.test.ts tools/compile-storage.test.ts` → **67 passed / 6 files** (slice 1 baseline; after fixing profile-equivalence test to `entrypoint:false`).
- `cd frontend && npx vitest run src/ide/ast-to-berry.test.ts src/ide/berry/source-map.test.ts src/editor/editor-document.test.ts` ??' **67 passed / 3 files** (statement+construct+control-flow+key-expression mappings, prefix-based/line-aware span mappings, default-parameter/concise-inline/block-body-inline/nested expression coverage, source-map lookup hardening, capture+remap mapping strategy, `annotateBerryErrorText` format hardening, editor summary checks).
- `cd frontend && npx vitest run src/console/console-lines.test.ts src/console/console-link.test.ts src/console/console-session.test.ts src/editor/editor-document.test.ts src/ide/berry/source-map.test.ts` → **19 passed / 5 files** (shared compile-context/console auto-annotation increment).
- `cd frontend && npx tsc --noEmit` → **passed**.
- `cd frontend && yarn lint` → **passed** (lint fixes: `ReadonlyArray`/simple-array typing in `editor-document.ts`, ownership-transfer eslint suppression in `berry-compile-context.ts`).
- `cd frontend && npm run check-storage` (`tools/compile-storage.test.ts`) → **3 passed / 1 file**.

No hardware validation required (compiler/editor-only). Firmware/runtime `BE_DEBUG_RUNTIME_INFO`/production-debug behavior remains a documented follow-up/design consideration.

## Follow-ups (R14 remaining)

- [ ] Refine compiler profiles with measured/dev-prod behavior decisions (e.g. comment/debug artifact policy if introduced later) while preserving semantics.
- [~] Improve mapping granularity further: **span-aware expression emitter/mapping propagation prototype (1a) implemented** for nested subexpressions in key statement forms; continue refining coverage/fidelity (all emission contexts, dedupe/source-position nuances, mapping volume/perf tradeoffs) and harden automatic runtime/Berry/device error mapping for real device output formats.
- [~] Wire annotated Berry/runtime/device error mapping to live runtime output when frontend receives it: **shared compile-metadata context (`BerryCompileContext`) + console transcript/pending auto-annotation implemented**; editor manual paste/annotate UX also available. Automatic usefulness depends on runtime output containing Berry line references and matching latest dev compile mappings.
- [~] Evaluate firmware/debug-runtime implications (`BE_DEBUG_RUNTIME_INFO`, production profiles, artifact size/startup): **evaluation/measurement plan documented** (`docs/reports/2026-11-10-r14-prod-profile-debug-runtime-evaluation.md`); no firmware config changes or hardware measurements performed yet.
- [ ] Editor memory diagnostics surfaced from runtime metrics: requires a defined frontend-accessible runtime metrics data source/API and UI integration (not implemented; `/api/heap` probe exists for console auth but response contract/UI are not established in editor).
- [ ] Update/add consolidated plan progress evidence and finalize R14 status when remaining acceptance criteria are met.
