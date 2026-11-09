import type ts from '@typescript/typescript6'

/**
 * Shared generation services. Statement and expression lowering depend on this
 * interface rather than on each other.
 */
export interface GenerationContext {
  readonly sourceFile: ts.SourceFile
  writeLine(text?: string): void
  indent(): void
  dedent(): void
  capture(build: () => void): string
  emitExpression(node: ts.Expression): string
  emitStatement(node: ts.Statement): void
  report(node: ts.Node, message: string): void
  declareName(name: string, node: ts.Node): void
  isDeclared(name: string): boolean
  pushScope(): void
  popScope(): void
  nextTemp(prefix: string): string
  noteDefaultExport(name: string, node: ts.Node): void
  noteNamedExport(exported: string, localExpr: string, node: ts.Node): void
}
