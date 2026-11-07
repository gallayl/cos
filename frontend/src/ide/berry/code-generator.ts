import ts from "@typescript/typescript6";

import type { GenerationContext } from "./context";
import {
  diagnosticAt,
  diagnosticText,
  readParseDiagnostics,
  type BerryDiagnostic,
  type BerryGenerationResult,
} from "./diagnostics";
import { emitExpression as lowerExpression } from "./expression-generator";
import { ScopeTracker } from "./scopes";
import { emitStatement as lowerStatement } from "./statement-generator";
import { BerryWriter } from "./writer";

class GenerationSession implements GenerationContext {
  readonly sourceFile: ts.SourceFile;
  private readonly writer = new BerryWriter();
  private readonly scopes = new ScopeTracker();
  private readonly diagnostics: BerryDiagnostic[] = [];
  private temps = 0;
  private exportedName: string | undefined;
  private readonly namedExports: Array<{ exported: string; local: string }> = [];

  constructor(sourceFile: ts.SourceFile) {
    this.sourceFile = sourceFile;
  }

  writeLine(text = ""): void {
    this.writer.writeLine(text);
  }

  indent(): void {
    this.writer.indent();
  }

  dedent(): void {
    this.writer.dedent();
  }

  capture(build: () => void): string {
    return this.writer.capture(build);
  }

  emitExpression(node: ts.Expression): string {
    return lowerExpression(this, node);
  }

  emitStatement(node: ts.Statement): void {
    lowerStatement(this, node);
  }

  report(node: ts.Node, message: string): void {
    const start = node.getStart(this.sourceFile, false);
    this.diagnostics.push(diagnosticAt(this.sourceFile, start, message));
  }

  declareName(name: string, node: ts.Node): void {
    if (!this.scopes.declare(name)) {
      this.report(node, `Redeclaring "${name}" is not supported.`);
    }
  }

  isDeclared(name: string): boolean {
    return this.scopes.isDeclared(name);
  }

  pushScope(): void {
    this.scopes.push();
  }

  popScope(): void {
    this.scopes.pop();
  }

  nextTemp(prefix: string): string {
    this.temps += 1;
    return `${prefix}${this.temps}`;
  }

  noteDefaultExport(name: string, node: ts.Node): void {
    if (this.exportedName && this.exportedName !== name) {
      this.report(node, "Only one default export is supported.");
      return;
    }
    this.exportedName = name;
  }

  noteNamedExport(exported: string, localExpr: string, _node: ts.Node): void {
    this.namedExports.push({ exported, local: localExpr });
  }

  toResult(): BerryGenerationResult {
    if (this.namedExports.length > 0) {
      this.writeLine("");
      const defaultValue = this.exportedName ?? "nil";
      const pairs = this.namedExports
        .map(({ exported, local }) => `${JSON.stringify(exported)}, ${local}`)
        .join(", ");
      const args = pairs.length > 0 ? `${defaultValue}, ${pairs}` : defaultValue;
      this.writeLine(`return cos_module(${args})`);
    } else if (this.exportedName) {
      this.writeLine("");
      this.writeLine(`return ${this.exportedName}`);
    }
    return {
      ok: this.diagnostics.length === 0,
      code: this.writer.toString(),
      diagnostics: this.diagnostics,
    };
  }
}

/**
 * @brief Generate Berry source from a TypeScript AST.
 * Parse failures produce diagnostics and no code. Unsupported syntax is reported
 * and never rewritten into a different runtime meaning.
 */
export function generateBerry(sourceFile: ts.SourceFile): BerryGenerationResult {
  const parseDiagnostics = readParseDiagnostics(sourceFile);
  if (parseDiagnostics.length > 0) {
    return {
      ok: false,
      code: "",
      diagnostics: parseDiagnostics.map((diagnostic) =>
        diagnosticAt(
          sourceFile,
          diagnostic.start ?? 0,
          diagnosticText(diagnostic.messageText),
        ),
      ),
    };
  }

  const session = new GenerationSession(sourceFile);
  for (const statement of sourceFile.statements) session.emitStatement(statement);
  return session.toResult();
}
