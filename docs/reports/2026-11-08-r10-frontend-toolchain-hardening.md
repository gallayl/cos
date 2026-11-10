# R10 — Frontend TS toolchain hardening

**Date:** 2026-11-09 (workspace date) / 2026-11-08 plan context  
**Plan item:** `docs/plan/consolidated-improvement-plan.md` R10  
**Scope:** R10 acceptance criteria except dedicated Web Worker compilation, which is explicitly deferred by user decision.

## Summary

R10 is completed with **Worker isolation deferred**. The frontend currently compiles TS→Berry on the main thread; this is verified, documented, and intentionally kept simple for the near/mid term because COS web UI apps are small and compilation is not expected to be a bottleneck.

## 1. Compilation / Worker isolation (deferred)

**Current behavior (verified):** `frontend/src/editor/editor-app.tsx` calls `astToBerry(getAst(document.text))` synchronously in `showCompileResult()` / Create Berry flows. Compilation runs on the main browser thread.

**Decision (2026-11-09, user-confirmed):** Do **not** move compilation to a dedicated Web Worker now. Rationale: COS apps built from the web UI are simple; worker/message-passing/cancellation/error plumbing adds complexity without clear near/mid-term benefit. Revisit if compile latency becomes noticeable, the supported TS subset grows, or multi-file compilation is added.

**Follow-up (optional):** If needed later, implement a Vite module worker around `getAst`/`ast-toBerry`, keep the same `TranspileResult` contract, and add UI states for pending/failed worker compilation.

## 2. Package/layout decision

**Decision:** Keep the integrated frontend layout:
- `frontend/src/cos-sdk/**` — COS/Berry declaration files and module typings.
- `frontend/src/ide/**` — TS workspace, AST/compiler, import resolution, file/device clients.
- `frontend/src/editor/**` — Shades UI/editor shell.

**Rationale:** Single frontend app, simple Vite/Vitest build, and no current external consumer requiring publishable packages. The original proposal’s `packages/cos-sdk` + `packages/cos-typescript` split remains a valid future extraction path when reuse (CLI compiler, SDK publication, multi-app tooling) justifies the packaging overhead.

## 3. Monaco language features and SDK `addExtraLib()` registration

**SDK registration (hardened/tested):**
- Added `frontend/src/ide/cos-sdk-manifest.ts` as the pure source of SDK libs, `@cos/*` module specifiers, and path mappings.
- `frontend/src/ide/cos-sdk-registration.ts` applies manifest data to Monaco TS/JS defaults (`noLib`, strict COS subset compiler options, explicit `paths` for `@cos/*` and aliases).
- Added `frontend/src/ide/cos-sdk-manifest.test.ts` to verify:
  - all global SDK declaration files are registered with stable virtual paths,
  - every `frontend/src/cos-sdk/modules/*.d.ts` file is covered (drift check via `import.meta.glob`),
  - every module specifier maps to its declaration path,
  - compatibility aliases remain (`@cos/fs`/`@cos/filesystem`, `@cos/net`/`@cos/network`).

**Monaco language features (audited, intentionally minimal):** `frontend/src/ide/monaco-languages.ts` registers CSS, HTML, JavaScript basic language definitions, Markdown, and JSON features. This is minimal for the text file types the editor can open (`editor-app.tsx` accepts `.ts/.js/.be/.json/.css/.html/.md/...` and common code/text formats). TypeScript/JavaScript language service features are provided through Monaco TS defaults used by SDK registration/workspace services.

## 4. Device filesystem read/write/deploy flows

**Audited paths:**
- `frontend/src/ide/files/cos-files.ts`: injected `fetchImpl`, Basic auth header propagation, `encodePathQuery` preserves `/`, API error parsing (`{error}` JSON, plain text, empty body fallback, HTML treated as unreachable-device/proxy response).
- `frontend/src/editor/device-access.ts`: shared auth state, `markUnauthorized()` on 401.
- `frontend/src/editor/editor-app.tsx`: save/deploy/create-Berry flows catch `CosFilesError` 401 and call `access.markUnauthorized()`; user-visible status uses `describeCosError`.
- `frontend/src/editor/file-browser.tsx` / `file-tree-view.tsx`: tree operations surface errors through activity callbacks and auth state (existing behavior retained).
- `frontend/src/editor/file-cache.ts`: directory/file caches key by `[path, authorization]`; failures remain on cache entries / reload catches are contained.

**Tests added/extended:** `frontend/src/ide/files/cos-files.test.ts` now covers `encodePathQuery` slash preservation, JSON `{error}` messages, HTML/empty failure bodies, and predictable propagation of auth/mutation failures for rename/upload.

## Validation

- `npx vitest run src/ide/files/cos-files.test.ts src/ide/cos-sdk-manifest.test.ts src/ide/cos-sdk.test.ts` — **18 passed / 3 files**.
- `npx tsc --noEmit` (frontend) — passed after test typing fix.
- Full frontend `vitest` / browser build not rerun here beyond targeted tests; recommend `cd frontend && npm test` and `npm run build` before release if desired.

## Hardware validation

Not required. R10 changes are frontend-only (TypeScript/Monaco/device API client/tests/docs). Device API behavior still depends on firmware endpoints, but this task did not change firmware or require physical CYD validation.
