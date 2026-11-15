---
name: add-device-script-example
description: Add or update a compiled device script example when a script-facing Berry/SDK API changes. Use when implementing or modifying APIs that on-device scripts can call and a TypeScript example plus compiled .be artifact must stay in sync.
---

# Add or compile a device script example

Use this skill when adding or changing a Berry API that scripts can call. Keep the TypeScript SDK and native binding in sync (see root `AGENTS.md`, "TypeScript SDK and Berry bindings") and deliver a compiled example.

## Workflow

1. **Identify the API surface.** Confirm the Berry name, argument order, and return value match in `frontend/src/cos-sdk/` declarations and the native registration in `components/berry_lang/src/` (and update both in the same change).
2. **Write the example in TypeScript.**
   - Put command scripts in `storage/ts/scripts/` (export/define a `main(args)` entry as used by the repo).
   - Put UI apps in `storage/ts/apps/`.
   - Exercise the new/changed API, including arguments when the command takes them.
3. **Compile with the COS Berry compiler (do not hand-translate).** Write the compiled output to the matching path under `storage/flash_data/`.
4. **Register expectations.** Add the script name to the expected list in `frontend/tools/compile-storage.test.ts`.
5. **Validate.** From `frontend/`, run `npm run check-storage` (and required frontend checks per root `AGENTS.md` when scope applies). Leave it passing or report actionable blockers.
6. **Report usage.** Tell the user the on-device/flash command to run (for example `berry exec scripts/font_size.be 2`).

## Example shape

```typescript
function main(args: string[]): number {
  const result = console.set_font_size(2);
  if (result !== true) {
    print(str(result));
    return 1;
  }
  return 0;
}
```

## Notes

- Prefer minimal examples that clearly demonstrate the API and its arguments/error cases.
- Do not claim compilation/checks passed unless executed; follow root `AGENTS.md` validation/reporting rules.
