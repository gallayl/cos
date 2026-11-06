/**
 * Types for the TypeScript subset that compiles to Berry.
 * Browser and Node globals (`console.log`, `window`, `Promise`, `Math`, …)
 * are intentionally absent. Device APIs live in the other COS declaration files.
 * `null` and `undefined` both compile to Berry `nil`.
 */

declare global {
  /**
   * Present so `for...of` accepts arrays.
   * Berry has no `Symbol` value; do not read or call it.
   */
  const Symbol: SymbolConstructor;

  interface SymbolConstructor {
    readonly iterator: unique symbol;
  }

  /** Berry list. `.length` compiles to `size()`. */
  interface Array<T> {
    /** Element count. Compiles to Berry `size()`. */
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

  /** Berry string. `.length` compiles to `size()`. */
  interface String {
    /** Byte length. Compiles to Berry `size()`. */
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
}

export {};
