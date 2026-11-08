/**
 * Berry `math` module (`import * as math from "@cos/math"` or `import math from "@cos/math"`).
 * Numeric values follow Berry runtime types (`int`/`real`, COS config: long/float).
 */
declare const _default: {
  [key: string]: any;
  pi: number;
  imin: int;
  imax: int;
  inf: number;
  nan: number;
  abs(x: number): number;
  ceil(x: number): number;
  floor(x: number): number;
  round(x: number): number;
  sqrt(x: number): number;
  exp(x: number): number;
  log(x: number): number;
  log10(x: number): number;
  pow(x: number, y: number): number;
  sin(x: number): number;
  cos(x: number): number;
  tan(x: number): number;
  asin(x: number): number;
  acos(x: number): number;
  atan(x: number): number;
  atan2(y: number, x: number): number;
  sinh(x: number): number;
  cosh(x: number): number;
  tanh(x: number): number;
  deg(x: number): number;
  rad(x: number): number;
  srand(seed: int): void;
  rand(): int;
  min(...values: number[]): number;
  max(...values: number[]): number;
  isnan(x: number): boolean;
  isinf(x: number): boolean;
};
export default _default;
