import type ts from '@typescript/typescript6'

/** Severity used by the IDE and compiler. Errors fail compilation. */
export type BerryDiagnosticSeverity = 'error' | 'warning' | 'info' | 'hint'

/** One rejection, rule violation, or parse failure, pointed at the original TypeScript. */
export interface BerryDiagnostic {
  message: string
  /** 1-based line in the TypeScript source. */
  line: number
  /** 1-based column in the TypeScript source. */
  column: number
  severity: BerryDiagnosticSeverity
  /** Optional stable rule/code identifier (for example `cos/no-top-level-exec`). */
  code?: string
}

/** Berry source plus diagnostics. Compilation fails when any error diagnostic exists. */
export interface BerryGenerationResult {
  /** False when any error-severity diagnostic exists. */
  ok: boolean
  code: string
  diagnostics: readonly BerryDiagnostic[]
}

/** @returns True when diagnostics contain at least one error. */
export function hasBerryErrors(diagnostics: readonly BerryDiagnostic[]): boolean {
  return diagnostics.some((diagnostic) => diagnostic.severity === 'error')
}

interface SourceFileWithParseDiagnostics extends ts.SourceFile {
  parseDiagnostics?: readonly ts.Diagnostic[]
}

/**
 * @brief Parse errors attached by the TypeScript parser, if any.
 */
export function readParseDiagnostics(sourceFile: ts.SourceFile): readonly ts.Diagnostic[] {
  return (sourceFile as SourceFileWithParseDiagnostics).parseDiagnostics ?? []
}

/**
 * @brief Point a diagnostic at a source offset.
 */
export function diagnosticAt(
  sourceFile: ts.SourceFile,
  start: number,
  message: string,
  severity: BerryDiagnosticSeverity = 'error',
  code?: string,
): BerryDiagnostic {
  const position = sourceFile.getLineAndCharacterOfPosition(Math.max(start, 0))
  return {
    message,
    line: position.line + 1,
    column: position.character + 1,
    severity,
    code,
  }
}

/**
 * @brief First message of a TypeScript diagnostic chain.
 */
export function diagnosticText(messageText: string | ts.DiagnosticMessageChain): string {
  return typeof messageText === 'string' ? messageText : messageText.messageText
}
