/**
 * Types for the TypeScript subset that compiles to Berry.
 * Browser and Node globals (`console.log`, `window`, `Promise`, `Math`, …)
 * are intentionally absent. Device APIs live in the other COS declaration files.
 *
 * Numeric model (Berry, pinned COS config): `int` is C `long` and `real` is C
 * `float`. TypeScript `number` may represent either Berry numeric runtime type.
 * Use explicit literals (`1` vs `1.0`) and `int()`/`real()` when intent matters.
 * Berry divides `int / int` with integer truncation; use `real(...)` for
 * TS/JS-style division.
 *
 * `null` and `undefined` both compile to Berry `nil`.
 */

declare global {
  /** Berry integer intent hint. Runtime numeric values are Berry `int` or `real`. */
  type int = number;
  /** Berry real intent hint. Runtime numeric values are Berry `int` or `real`. */
  type real = number;

  /**
   * Present so `for...of` accepts arrays.
   * Berry has no `Symbol` value; do not read or call it.
   */
  const Symbol: SymbolConstructor;

  interface SymbolConstructor {
    readonly iterator: unique symbol;
  }

  /** Berry list. `.length` compiles to Berry global `size(value)`. */
  interface Array<T> {
    /** Element count. Compiles to Berry global `size(value)`. */
    readonly length: number;
    [index: number]: T;
    /**
     * Append one item. Compiles to Berry `list.push`, which returns nil.
     */
    push(item: T): void;
    [Symbol.iterator](): Iterator<T>;
  }

  interface Iterator<T> {
    next(): IteratorResult<T>;
  }

  interface IteratorYieldResult<T> {
    done?: false;
    value: T;
  }

  interface IteratorReturnResult<T> {
    done: true;
    value: T;
  }

  type IteratorResult<T> = IteratorYieldResult<T> | IteratorReturnResult<T>;

  /** Berry string. `.length` compiles to Berry global `size(value)`. */
  interface String {
    /** Byte length. Compiles to Berry global `size(value)`. */
    readonly length: number;
  }

  /**
   * Required by the type checker for primitive names.
   * Methods are omitted because this subset does not compile them.
   */
  interface Boolean {}
  interface Number {}
  interface Object {}
  interface RegExp {}
  interface IArguments {}
  interface Function {}
  interface CallableFunction extends Function {}
  interface NewableFunction extends Function {}

  /**
   * Berry VM `print`. Writes to the Berry output stream shown by the shell.
   * This is not JavaScript `console.log`.
   */
  function print(message: string): void;

  /**
   * Berry VM `str()`. Turns a value into its display text.
   * Template substitutions compile to this call. `str` of a string is that string.
   */
  function str(value: string | number | boolean | null | undefined): string;

  /** Berry global `int()`. Converts values; may return `nil` for unsupported values. */
  function int(value: unknown): int;
  /** Berry global `real()`. Converts values; may return `nil` for unsupported values. */
  function real(value: unknown): real;
  /** Berry global `bool()`. Returns Berry truthiness as `true`/`false`. */
  function bool(value: unknown): boolean;
  /** Berry global `size()`. String byte length, or `value.size()` when available. */
  function size(value: unknown): int;
  /** Berry global `type()`. Returns Berry type name (`int`, `real`, `string`, `list`, `map`, `instance`, `nil`, ...). */
  function type(value: unknown): string;
}

export {};
