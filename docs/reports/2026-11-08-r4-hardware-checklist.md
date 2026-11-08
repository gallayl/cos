# R4 — `try`/`catch`/`throw` Compiler Lowering Hardware Checklist

**Date:** 2026-11-08  
**Related:** `docs/2026-11-08-consolidated-improvement-plan.md` (R4), `docs/2026-11-07-ts-feature-extensions.md`, `frontend/src/ide/berry/statement-generator.ts`, `frontend/src/ide/ast-to-berry.test.ts`  
**Goal:** Validate that TypeScript `try`/`catch` and `throw` compile to Berry exception handling and behave correctly on the target Berry VM/device.

## What changed

- `throw <expr>` lowers to Berry `raise <expr>`.
- `try { ... } catch (e) { ... }` lowers to Berry `try ... except .. as e ... end`.
- `try { ... } catch { ... }` (optional catch binding) lowers to `try ... except .. ... end`.
- Catch binding type annotations are erased (`catch (e: unknown)` is accepted).
- Diagnostics (compile-time, no silent miscompile):
  - `finally` is not supported (`try/finally`, `try/catch/finally`).
  - Destructuring catch bindings are not supported (`catch ({ message })`).
  - A `try` without a `catch` clause is rejected by the compiler.

## Berry semantics notes

- Berry uses `raise` for exceptions. Raising a string is valid (`raise "stop_iteration"` is used in Berry tests).
- `except .. as e` catches any exception and binds the Berry exception value to `e`. For Berry built-in errors, the first value is often the error type string (for example `assert_failed`) and Berry can also provide a message as a second parameter (`except .. as e, m`). The TS compiler currently maps a single catch identifier to the first Berry exception value only.
- Berry has no `finally`. Cleanup must be explicit (duplicate cleanup, helper functions, or state flags) until/unless a safe lowering strategy is designed.
- Normal control flow inside `try`/`except` blocks (`return`, `break`, `continue`) is emitted as normal Berry statements.

## Preconditions

- [ ] Build/flash firmware from the current tree (or run a compatible Berry VM). R4 is a frontend compiler change; the generated `.be` output is what runs on device.
- [ ] Have a workflow to compile TS to Berry and deploy/run it:
  - IDE/Monaco TS editor with the current `frontend/src/ide/berry/*` compiler, or
  - a local script/test harness that produces `.be` output, then copy to `/flash` or `/sdcard` and run with `berry run ...`.
- [ ] Note device, build target, commit, and network/BLE state.
- [ ] Capture `berry profile` / `mem` before and after tests if you want to spot obvious regressions (R4 should not change steady-state memory).

## 1. Basic throw/catch

Deploy and run a generated script equivalent to:

```ts
function main(): void {
  try {
    throw "boom";
  } catch (e) {
    print("caught " + str(e));
  }
  print("after");
}
main();
```

Expected Berry shape:

```berry
def main()
    try
        raise "boom"
    except .. as e
        print("caught " + str(e))
    end
    print("after")
end
main()
```

- [ ] Output contains `caught boom` (or equivalent string form) and `after`.
- [ ] No Berry exception escapes to the shell/runtime.

## 2. Catch without binding

```ts
function main(): void {
  try {
    throw "x";
  } catch {
    print("handled");
  }
}
main();
```

- [ ] Output contains `handled`.
- [ ] Generated Berry uses `except ..` (no `as ...`) for the catch clause.

## 3. Rethrow

```ts
function main(): void {
  try {
    try {
      throw "inner";
    } catch (e) {
      print("inner=" + str(e));
      throw e;
    }
  } catch (e) {
    print("outer=" + str(e));
  }
}
main();
```

- [ ] Output shows inner catch, then outer catch with the same exception value.
- [ ] No crash/hang.

## 4. Control flow inside try/catch

```ts
function firstPositive(items: number[]): number {
  for (const item of items) {
    try {
      if (item > 0) {
        return item;
      }
    } catch (e) {
      return -99;
    }
  }
  return -1;
}

print(firstPositive([-1, 0, 7, 9]));
```

- [ ] Prints `7`.
- [ ] Returning from inside `try` behaves normally.

## 5. Berry built-in/assert-style exception observation (optional)

If convenient, exercise a case that triggers a Berry runtime exception and inspect the bound value:

```ts
function main(): void {
  try {
    // Intentionally trigger an error path appropriate for your runtime;
    // adjust if this specific call is unavailable.
    import("json").load("not-json");
  } catch (e) {
    print("e=" + str(e));
  }
}
main();
```

- [ ] Script catches the exception and continues (exact `e` string may vary by Berry/runtime error).
- [ ] Record the observed `e` value. This helps confirm whether users need access to Berry's second exception message parameter in a future compiler/SDK follow-up.

## 6. Compile-time diagnostics (IDE/compiler UI)

These should **not** produce runnable Berry output as successful compilations:

- [ ] `try { work(); } finally { cleanup(); }` reports `try/finally is not supported...`
- [ ] `try { work(); } catch (e) { } finally { cleanup(); }` reports `try/finally is not supported...`
- [ ] `try { work(); } catch ({ message }) { log(message); }` reports `Destructuring in catch bindings is not supported...`

## 7. Sanity / memory

- [ ] Running the generated R4 test scripts does not crash the Berry VM, corrupt heap, or trigger watchdog resets.
- [ ] `berry profile` before/after repeated runs shows no unexpected progressive leak attributable to R4-generated code (small variation is normal).

## Record results

| Item | Result | Notes |
|---|---|---|
| Basic throw/catch | ✅ Pass | Output: `[1] caught boom`, `[1] after` |
| Catch without binding | ✅ Pass | Output: `[2] handled` |
| Rethrow | ✅ Pass | Output: `[3] inner=oops`, `[3] outer=oops` |
| Control flow in try/catch | ✅ Pass | Output: `[4] 7`, `[5] loop caught at 0`, `[5] done`; `return` in try and `break` in catch behaved correctly |
| Built-in exception observation | ⬜ Optional | Not recorded in this run |
| Compiler diagnostics | ⬜ Not recorded | Covered by frontend unit tests (`finally`, `try/catch/finally`, destructuring catch binding) |
| Sanity/memory | ✅ Pass (smoke) | No uncaught Berry exception, crash, or watchdog reset observed; ended with `R4 OK` |

**Tester:** User  
**Date:** 2026-11-08  
**Command:** `berry run /sdcard/tests/trycatch.be`  
**Observed output:**

```text
[1] caught boom
[1] after
[2] handled
[3] inner=oops
[3] outer=oops
[4] 7
[5] loop caught at 0
[5] done
R4 OK
```

**Follow-ups filed:** Consider exposing Berry exception message (second `except` arg, e.g. `except .. as e, m`) if SDK/runtime error handling needs it.
