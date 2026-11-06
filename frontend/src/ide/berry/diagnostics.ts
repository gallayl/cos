import type ts from "@typescript/typescript6";

/** One rejection or parse failure, pointed at the original TypeScript. */
export interface BerryDiagnostic {
  message: string;
  /** 1-based line in the TypeScript source. */
  line: number;
  /** 1-based column in the TypeScript source. */
  column: number;
}

/** Berry source plus any reasons it must not be executed. */
export interface BerryGenerationResult {
  /** False when the source uses syntax this generator will not lower. */
  ok: boolean;
  code: string;
  diagnostics: readonly BerryDiagnostic[];
}

interface SourceFileWithParseDiagnostics extends ts.SourceFile {
  parseDiagnostics?: readonly ts.Diagnostic[];
}

/**
 * @brief Parse errors attached by the TypeScript parser, if any.
 */
export function readParseDiagnostics(
  sourceFile: ts.SourceFile,
): readonly ts.Diagnostic[] {
  return (
    (sourceFile as SourceFileWithParseDiagnostics).parseDiagnostics ?? []
  );
}

/**
 * @brief Point a diagnostic at a source offset.
 */
export function diagnosticAt(
  sourceFile: ts.SourceFile,
  start: number,
  message: string,
): BerryDiagnostic {
  const position = sourceFile.getLineAndCharacterOfPosition(Math.max(start, 0));
  return {
    message,
    line: position.line + 1,
    column: position.character + 1,
  };
}

/**
 * @brief First message of a TypeScript diagnostic chain.
 */
export function diagnosticText(
  messageText: string | ts.DiagnosticMessageChain,
): string {
  return typeof messageText === "string" ? messageText : messageText.messageText;
}
