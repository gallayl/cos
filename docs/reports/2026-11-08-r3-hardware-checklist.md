# R3 — TS→Berry Semantic Conformance Hardware Checklist

**Date:** 2026-11-08  
**Related:** `docs/plan/consolidated-improvement-plan.md` (R3), `docs/history/2026-11-08-ts-berry-semantic-conformance.md`  
**Goal:** Validate that R3 compiler/SDK changes behave correctly on the target Berry VM/device, especially generated code that now uses Berry global `size(...)` and preserves numeric literal lexical form.

## What changed (why validate)

- `items.length` / `text.length` now compile to `size(items)` / `size(text)` (Berry global), not `items.size()` / `text.size()`. This fixes Berry string length semantics and remains compatible with lists/maps/bytes via `be_baselib_size`.
- Numeric literals preserve source lexical form (`2.0`, `1e3`) with `_` separators removed.
- Compiler now diagnoses `int-literal / int-literal` (Berry integer division ambiguity), unary `+`, and gives clearer `typeof`/`void`/`delete` diagnostics. (Diagnostics are compile-time; no runtime change except preventing ambiguous source from being accepted.)
- Regenerated stored Berry outputs: `storage/flash_data/apps/breakout.be`, `storage/flash_data/scripts/arg_test.be`, `fetch_json.be`, `files.be`, `font_size.be`, `host_info.be`, `set_base.be`, `wget.be`, `wifi.be` (and any others touched by `compile-storage --write`).

## Preconditions

- [ ] Build/flash firmware that includes the current tree (or ensure the device is running a compatible Berry VM; compiler output is static Berry source).
- [ ] Deploy updated `storage/flash_data` contents to the device/filesystem as you normally do (especially regenerated `.be` scripts/apps). If you only flash firmware, the bundled/generated flash data may not update depending on your workflow.
- [ ] Note device, build target, commit, and whether BLE/Wi-Fi are active (heap-sensitive).

## 1. Basic Berry numeric model sanity (shell / Berry eval)

Run in the Berry shell/console where available:

- [ ] `print(size("abc"))` → `3` (or expected byte length). Confirms global `size()` on strings.
- [ ] `print(size([1,2,3]))` → `3`. Confirms global `size()` on lists.
- [ ] `print(size({"a":1,"b":2}))` → `2` (if map `size` works in your Berry config). 
- [ ] `var m = {"a":1}; print(m.contains("a")); print(m.find("a")); print(m.find("z", 9))` → `true`, `1`, `9` (or equivalent). Confirms Berry map semantics (not JS `Map.has/get`).
- [ ] `print(1 / 2)` → `0` (int/int). Confirms Berry integer division.
- [ ] `print(1.0 / 2)` → `0.5` (or float form). Confirms real division when a real operand is present.
- [ ] `print(real(1) / real(2))` → `0.5`. Confirms explicit conversion path recommended by SDK.
- [ ] `print(int(3.9))` → `3` (or Berry's truncation behavior; note C-cast style). Confirms `int()` behavior.
- [ ] `print(type(1)); print(type(1.0)); print(type(nil)); print(type("x")); print(type([1])); print(type({"a":1}))` → expected Berry type names (`int`, `real`, `nil`, `string`, `list`, `map`/`instance` depending on representation).

Record any differences (especially if Berry build/config differs from `BE_INTGER_TYPE=1`, `BE_USE_SINGLE_FLOAT=1`).

## 2. Regenerated storage scripts (behavior unchanged, safer `size(...)`)

These scripts previously used `args.size()`-style lowering and now use `size(args)`. Test normal usage:

- [ ] `arg_test` / equivalent: run with 0, 1, and multiple args. Argument count/index output should be unchanged.
- [ ] `files`: no-arg usage/help and normal listing behavior unchanged.
- [ ] `fetch_json <url>` (if network available): argument validation unchanged; JSON fetch path still works or fails with expected network errors.
- [ ] `font_size 1|2|3`: argument validation and font-size change unchanged.
- [ ] `host_info`: no-arg behavior unchanged.
- [ ] `set_base <url>` / `wget <url>` / `wifi` subcommands: argument parsing/count checks unchanged.
- [ ] `breakout` app: launches, bricks render, input works, no crash/regression from `size(self.bricks)` lowering.

For each: note pass/fail and any log/error output.

## 3. TS→Berry semantic mappings (compile a small sample if IDE/device workflow supports it)

If you can compile TS in the frontend/IDE and run the generated Berry on device, validate:

- [ ] **String length:** TS `text.length` in a compiled script returns expected length for ASCII; note behavior for non-ASCII if relevant (Berry `size()` on strings is byte length in C/implementation terms).
- [ ] **Null/undefined checks:** `value == null`, `value === null`, `value === undefined` behave as nil checks after compilation.
- [ ] **Strict equality:** compiled `===`/`!==` behaves as Berry `==`/`!=`.
- [ ] **Template strings:** interpolation produces expected string output.
- [ ] **Class init fields:** default-initialized fields have expected values before/after `setup`.
- [ ] **Division intent:** a TS sample using `real(a) / real(b)` or `1.0 / 2.0` produces expected non-truncated result. (Compiler should reject `1 / 2` source form before deploy.)
- [ ] **Conditions:** explicit comparisons (`count > 0`, `text != ""`, `value != nil`) work; truthy shorthand patterns that the compiler rejects are not present in deployed code.

## 4. Memory/sanity spot check

- [ ] Run `mem` / `berry profile` (if available) before and after opening/running regenerated scripts/apps. No unexpected large drop attributable to R3 changes.
- [ ] No new watchdog resets, heap corruption, or Berry exceptions when running regenerated scripts.

## 5. Record results

Fill in when validating:

| Item | Result | Notes |
|---|---|---|
| Berry numeric model sanity | ⬜ | |
| Regenerated storage scripts/apps | ⬜ | |
| TS semantic sample(s) | ⬜ | |
| Memory/sanity | ⬜ | |

**Tester:**  
**Date:**  
**Commit/device:**  
**Follow-ups filed:**  
