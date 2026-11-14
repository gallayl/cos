import type ts from '@typescript/typescript6'

import type { BerrySourceMapEntry } from './diagnostics'

/**
 * Shared generation services. Statement and expression lowering depend on this
 * interface rather than on each other.
 */
export interface GenerationContext {
  readonly sourceFile: ts.SourceFile
  writeLine(text?: string): void
  indent(): void
  dedent(): void
  capture(build: () => void): { text: string; mappings: readonly BerrySourceMapEntry[] }
  emitExpression(node: ts.Expression): string
  emitStatement(node: ts.Statement): void
  report(node: ts.Node, message: string, severity?: 'error' | 'warning' | 'info' | 'hint', code?: string): void
  declareName(name: string, node: ts.Node): void
  isDeclared(name: string): boolean
  pushScope(): void
  popScope(): void
  nextTemp(prefix: string): string
  noteDefaultExport(name: string, node: ts.Node): void
  noteNamedExport(exported: string, localExpr: string, node: ts.Node): void
  /**
   * Record a best-effort debug mapping at the current generated output position.
   * `columnOffset` is the 0-based offset from the statement indent/start column.
   */
  noteMapping(node: ts.Node, columnOffset?: number): void
  /**
   * Record a mapping at an explicit generated position relative to the next/current writer line.
   * `lineDelta` is 0-based lines after the writer's next line.
   * `columnOffset` is 0-based from the statement/writer indent start (added to writer indent column).
   */
  noteMappingAt(node: ts.Node, lineDelta: number, columnOffset: number): void
}
