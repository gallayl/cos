# TS→Berry Semantic Conformance Matrix (R3)

**Date:** 2026-11-08  
**Scope:** `frontend/src/ide/berry/*` compiler, `frontend/src/cos-sdk/*` SDK declarations, and Berry runtime semantics pinned by `components/berry_lang/CMakeLists.txt` (`65fce31732a9bdb91cb42cfc86e2b40f16c57514`, `BE_INTGER_TYPE=1` long, `BE_USE_SINGLE_FLOAT=1`).

This matrix documents what the COS TypeScript subset means when lowered to Berry. Constructs not listed are either unsupported (diagnosed) or not yet audited.

## Core value mappings

| TypeScript | Berry | Notes / tests |
|---|---|---|
| `true` / `false` | `true` / `false` | Direct keyword mapping. |
| `null` | `nil` | `NullKeyword` → `nil`. |
| `undefined` (undeclared identifier) | `nil` | Only when `undefined` is not a declared name in scope; avoids rewriting user declarations named `undefined`. Redundant checks like `x === null || x === undefined` become `x == nil || x == nil` (correct, not optimized). |
| `this` | `self` | Class methods/arrows lowered in class context. |
| `\"...\"` / `'...'` strings | Berry double-quoted strings | Cooked text escaped (`\\`, `"`, `\n`, `\r`, `\t`). No raw/binary string model. |
| Template literals | `str(part) + ...` concatenation | Substitutions compile to Berry global `str(value)`. `str(string)` returns the string in Berry. |
| Numeric literals | Same numeric text, `_` separators removed | `1` → Berry `int` literal, `1.0` → Berry `real` literal as written. Berry distinguishes `int` and `real` at runtime. |
| Arrays `[a, b]` | `[a, b]` list literal | Array holes and spread diagnosed. |
| Objects `{ a: 1, "b": 2 }` | `{"a": 1, "b": 2}` map literal | Keys emitted as Berry strings. Computed keys/shorthand/spread/getter/setter/method properties diagnosed. |

## Numeric model (Berry)

Berry (pinned config) has two numeric runtime types:

- **`int`**: C `long` (`BE_INTGER_TYPE=1`). Range exposed as `math.imin`/`math.imax`.
- **`real`**: C `float` (`BE_USE_SINGLE_FLOAT=1`).

TypeScript has only `number` (IEEE 754 double semantics in JS engines). The compiler cannot represent all TS numeric behavior exactly on constrained hardware. The SDK should make Berry numeric intent explicit where it matters.

| Operation | Berry behavior | TS authoring guidance |
|---|---|---|
| `int` literal (`42`) | `int` | Use for counts, indices, IDs where integer semantics are intended. |
| `real` literal (`42.0`, `1e3`) | `real` | Use when real value is intended. |
| `int +-*% int` | `int` | Same family as TS for safe integer ranges. |
| mixed `int`/`real` arithmetic (`+ - *`) | `real` | Berry promotes to real for numeric binops where applicable. |
| `int / int` | **integer division** in Berry VM, raises `divzero_error` on zero | **Differs from TS/JS** (`1 / 2` is `0` in Berry, `0.5` in TS). Do not rely on JS float division for integer operands; use `real(a) / real(b)` or `a * 1.0 / b` when real division is required. |
| number `/` number with real operand | `real` division | Matches TS intent better. |
| `%` | Berry modulo operator for ints (VM binop rules) | Use with integer operands; avoid JS-specific float remainder assumptions. |
| `int(x)`, `real(x)`, `number(x)` | Berry builtins | Available in global scope / SDK declarations. `int` truncates reals toward zero via C cast in Berry impl; `real` converts ints/real strings. |
| `math.*` | `math` module (native); many functions return/push `real` | Import from `@cos/math` when not using eager globals in future; currently global/module availability depends on runtime. |

**Compiler status (implemented in R3):**
- Numeric literals preserve source form (so `2.0`/`1e3` remain real lexically in Berry) with `_` separators removed.
- `/` and `%` lower to Berry operators without coercion.
- `int-literal / int-literal` (lexical ints, after removing `_`; excluding `.`/`e`/hex/bin/oct reals) is diagnosed because Berry performs integer division for `int / int`. Use `real(...)`, `1.0 / 2.0`, or variables. `int(x) / int(y)` is accepted (calls are not numeric literals).
- Unary `+` is diagnosed; use `int(x)`/`real(x)`.
- `typeof`, `void`, and `delete` are diagnosed as unsupported Berry mappings.

## Operators

| TypeScript | Berry | Status |
|---|---|---|
| `=` `+=` `-=` `*=` `/=` `%=` | same tokens | Supported. Right-associative assignment precedence preserved. |
| `+ - * / %` | `+ - * / %` | Supported. See numeric model for `/` and `%`. `+` is numeric/string concatenation in Berry depending on operands; TS author must ensure intended types. |
| unary `-x` | `-x` | Supported. |
| unary `!x` | `!x` | Supported only for structurally boolean operands (R3 rule); Berry `!` negates Berry truthiness of bool/nil? Condition rules are stricter than Berry runtime to avoid TS truthiness traps. |
| unary `+x` | — | Diagnosed unsupported (use `real(x)`, `int(x)`, or numeric expression explicitly). |
| `===` / `!==` | `==` / `!=` | Supported. |
| `==` / `!=` (loose) | — | Diagnosed: use strict equality. |
| `< > <= >=` | same | Supported. |
| `&&` / `||` | same | Supported for structurally boolean operands; TS short-circuit value semantics differ from strict boolean discipline. |
| `??`, `?.` | — | Diagnosed unsupported. |
| bitwise, `**`, `instanceof`, `in`, comma, `delete`, `typeof`, `void` | — | Diagnosed unsupported (or fall through to unsupported syntax). |
| `++` / `--` | `x += 1` / `x -= 1` as statements | Supported only as statements, not expression values. |

## Conditions and truthiness

Berry treats only `false` and `nil` as falsy. TypeScript/JS treat `0`, `NaN`, `""` (and `null`/`undefined`) as falsy. The compiler **rejects** conditions/boolean operands it cannot prove safe:

- Allowed structurally: boolean literals, comparisons, `&&`/`||` of boolean structures, `!` of boolean structure, identifiers, calls, `new`, element access, and property access **except** `.length` (because `.length` is lowered and represents a count, not an explicit boolean comparison).
- Rejected with guidance: numeric/string literals in boolean position, `.length` in boolean position, and other ambiguous forms. Use explicit comparisons (`count > 0`, `text != ""`, `value != nil`).

## Size / length

| TypeScript | Berry | Notes |
|---|---|---|
| `arr.length` (not called) | `size(arr)` | Uses Berry global `size()`, which returns string byte length for strings and calls `obj.size()` for objects that support it (list, map, bytes, etc.). This is more compatible than `arr.size()` for strings. |
| `str.length` (not called) | `size(str)` | Correct for Berry strings; `str.size()` is not a general Berry string method. |
| `expr.length()` as call | `expr.length()` | Not rewritten (treated as normal call/property callee path). Unusual in TS; Berry target must define it. |
| JS `Map.size` property style | — | No special lowering; Berry maps use `size()` method/global. Prefer `size(map)` / explicit API patterns and document SDK expectations. |

## Strings, lists, maps

- **Strings:** indexing and range behavior follow Berry (`s[i]`, `s[a..b]`). `size(s)` returns byte length. String mutation APIs are Berry/module-specific, not JS `String.prototype`.
- **Lists:** Berry list methods include `size`, `push`, `pop`, `insert`, `remove`, `item`, `setitem`, `find` (returns index or `nil`), `concat`, `reverse`, `copy`, `keys`, `clear`, `resize`, `iter`, `tobool`, `==`/`!=`. TS `Array` methods are **not** auto-mapped except `.length` → `size(...)`.
- **Maps:** Berry map methods include `size`, `find(key[, default])` (returns value/default, not boolean presence), `contains`, `insert`, `remove`, `item`/`setitem`, `iter`, `keys`. JS `Map.get/set/has/size` semantics are **not** auto-mapped.

## Functions and classes

- Function declarations/arrows/classes lower to Berry `def`/`class` forms with lexical-scope redeclaration checks.
- `const`/`let` become `var` declarations (Berry scope differs; redeclaration is diagnosed).
- Class fields/constructor initialize `self.*` in `init`.
- `return` with/without value supported.
- Unsupported: generators, async, `super`, accessors, static members/blocks, private identifiers, decorators, parameter properties, destructuring, default/rest patterns beyond supported forms, `arguments`, `yield`, `await`.

## Control flow

Supported: `if`/`else if`/`else`, `while`, C-style `for` (lowered to while), `for..of`, `switch` (lowered to if/elif chain with temp, no fallthrough), `break`/`continue` (no labels), blocks, expression/return statements.

Supported (R4, 2026-11-08, compiler): `try`/`catch` lowers to Berry `try`/`except .. [as e]`/`end`; `throw` lowers to Berry `raise`. A single identifier catch binding (optional type annotation erased) is supported; optional catch binding (`catch {}`) is supported. Berry binds the first exception value to `e` in the supported form (Berry also supports an optional second message parameter in `except .. as e, m`, not currently exposed by TS catch syntax).

Diagnosed: `for..in`, `for await`, `do..while`, labeled break/continue, `try`/`finally` / `try`/`catch`/`finally` (Berry has no `finally`), destructuring catch bindings, `with`, `debugger`, `switch` fallthrough.

## Imports/exports

Type-only imports/exports erased. Relative imports/exports and `@cos/*`/`@berry/*` module specifiers lower to `cos_import(...)`/`cos_module(...)` runtime helpers as implemented. Non-string specifiers, side-effect imports, unsupported re-export forms, and bare package imports are diagnosed.

## Known follow-ups (outside R3 minimum / verify)

- Consider a weaker/optional diagnostic mode for integer division when variable types are unknown but author intent is clear.
- Audit `string` module function names/signatures against Berry `string.*` and avoid implying TS `String.prototype` methods are available.
- Consider `console.log` vs Berry `print`/COS `log()` mapping in SDK docs (Berry `print` is global; COS also has `log()` native binding).
- R4: `try`/`catch`/`throw` compiler lowering complete (2026-11-08), including hardware validation; `finally` remains diagnosed. Follow-up: consider exposing Berry exception message (second `except` binding) if needed by SDK/runtime error model. R18: `async`/`await`.
- R6: runtime lazy loading for non-JSON modules; ensure `@cos/*` imports align.
- Hardware validation: run generated Berry apps/scripts that exercise `size()`, numeric literals/division, `nil` checks, template strings, and class init on device/VM.
