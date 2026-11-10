import ts from '@typescript/typescript6'

import type { CosClassInfo } from './class-generator'
import { emitClass } from './class-generator'
import type { GenerationContext } from './context'
import {
  diagnosticAt,
  diagnosticText,
  hasBerryErrors,
  readParseDiagnostics,
  type BerryCompileMetadata,
  type BerryCompileProfile,
  type BerryDiagnostic,
  type BerryDiagnosticSeverity,
  type BerryGenerationResult,
  type BerrySourceMapEntry,
} from './diagnostics'
import { emitExpression as lowerExpression } from './expression-generator'
import { ScopeTracker } from './scopes'
import { emitStatement as lowerStatement } from './statement-generator'
import { BerryWriter } from './writer'

class GenerationSession implements GenerationContext {
  readonly sourceFile: ts.SourceFile
  readonly entrypoint: boolean
  readonly profile: BerryCompileProfile
  private readonly writer = new BerryWriter()
  private readonly scopes = new ScopeTracker()
  private readonly diagnostics: BerryDiagnostic[] = []
  private readonly mappings: BerrySourceMapEntry[] = []
  private readonly mappingKeys = new Set<string>()
  private temps = 0
  private exportedName: string | undefined
  private readonly namedExports: Array<{ exported: string; local: string }> = []
  private defaultClassInfo: CosClassInfo | undefined
  private captureDepth = 0
  private readonly captureMappingBuffers: Array<Array<BerrySourceMapEntry & { __node: ts.Node }>> = []

  constructor(sourceFile: ts.SourceFile, entrypoint: boolean, profile: BerryCompileProfile) {
    this.sourceFile = sourceFile
    this.entrypoint = entrypoint
    this.profile = profile
  }

  writeLine(text = ''): void {
    this.writer.writeLine(text)
  }

  indent(): void {
    this.writer.indent()
  }

  dedent(): void {
    this.writer.dedent()
  }

  capture(build: () => void): { text: string; mappings: readonly BerrySourceMapEntry[] } {
    this.captureDepth += 1
    const buffer: Array<BerrySourceMapEntry & { __node: ts.Node }> = []
    this.captureMappingBuffers.push(buffer)
    try {
      const text = this.writer.capture(build)
      return { text, mappings: buffer }
    } finally {
      this.captureMappingBuffers.pop()
      this.captureDepth -= 1
    }
  }

  emitExpression(node: ts.Expression): string {
    return lowerExpression(this, node)
  }

  emitStatement(node: ts.Statement): void {
    this.recordMapping(node)
    lowerStatement(this, node)
  }

  report(node: ts.Node, message: string, severity: BerryDiagnosticSeverity = 'error', code?: string): void {
    const start = node.getStart(this.sourceFile, false)
    this.diagnostics.push(diagnosticAt(this.sourceFile, start, message, severity, code))
  }

  declareName(name: string, node: ts.Node): void {
    if (!this.scopes.declare(name)) {
      this.report(node, `Redeclaring "${name}" is not supported.`, 'error', 'cos/redeclare')
    }
  }

  isDeclared(name: string): boolean {
    return this.scopes.isDeclared(name)
  }

  pushScope(): void {
    this.scopes.push()
  }

  popScope(): void {
    this.scopes.pop()
  }

  nextTemp(prefix: string): string {
    this.temps += 1
    return `${prefix}${this.temps}`
  }

  noteDefaultExport(name: string, node: ts.Node): void {
    if (this.exportedName && this.exportedName !== name) {
      this.report(node, 'Only one default export is supported.', 'error', 'cos/one-default-export')
      return
    }
    this.exportedName = name
  }

  noteNamedExport(exported: string, localExpr: string, _node: ts.Node): void {
    this.namedExports.push({ exported, local: localExpr })
  }

  get hasDefaultClass(): boolean {
    return this.defaultClassInfo !== undefined
  }

  get hasErrors(): boolean {
    return hasBerryErrors(this.diagnostics)
  }

  emitTopLevelClass(node: ts.ClassDeclaration): void {
    this.recordMapping(node)
    const info = emitClass(this, node)
    if (info?.isDefaultExport) this.defaultClassInfo = info
  }

  toResult(): BerryGenerationResult {
    if (this.entrypoint) {
      this.emitEntrypointLowering()
    } else if (this.namedExports.length > 0) {
      this.writeLine('')
      const defaultValue = this.exportedName ?? 'nil'
      const pairs = this.namedExports.map(({ exported, local }) => `${JSON.stringify(exported)}, ${local}`).join(', ')
      const args = pairs.length > 0 ? `${defaultValue}, ${pairs}` : defaultValue
      this.writeLine(`return cos_module(${args})`)
    } else if (this.exportedName) {
      this.writeLine('')
      this.writeLine(`return ${this.exportedName}`)
    }
    return {
      ok: !hasBerryErrors(this.diagnostics),
      code: this.writer.toString(),
      diagnostics: this.diagnostics,
      metadata: this.buildMetadata(),
    }
  }

  private buildMetadata(): BerryCompileMetadata {
    const notes: string[] = [
      this.profile === 'dev'
        ? 'Dev profile: readable Berry output with statement-level debug mappings.'
        : 'Prod profile: readable Berry output with reduced debug metadata.',
      'Berry runtime line/stack debug details depend on firmware build options (for example BE_DEBUG_RUNTIME_INFO). This compiler metadata maps generated Berry locations to TypeScript source.',
    ]
    return {
      profile: this.profile,
      sourceFileName: this.sourceFile.fileName || undefined,
      mappings: this.profile === 'dev' ? this.mappings : [],
      notes,
    }
  }

  noteMapping(node: ts.Node, columnOffset = 0): void {
    this.recordMapping(node, columnOffset)
  }

  noteMappingAt(node: ts.Node, lineDelta: number, columnOffset: number): void {
    this.recordMappingAt(node, lineDelta, columnOffset)
  }

  private recordMapping(node: ts.Node, columnOffset = 0): void {
    this.recordMappingAt(node, 0, Math.max(0, columnOffset))
  }

  private recordMappingAt(node: ts.Node, lineDelta: number, columnOffset: number): void {
    if (this.profile !== 'dev') return
    const start = node.getStart(this.sourceFile, false)
    const sourcePos = this.sourceFile.getLineAndCharacterOfPosition(Math.max(start, 0))
    const generated = {
      line: this.writer.nextLine + Math.max(0, lineDelta),
      column: Math.max(1, this.writer.nextColumn + Math.max(0, columnOffset)),
    }
    const source = { line: sourcePos.line + 1, column: sourcePos.character + 1 }
    const entry: BerrySourceMapEntry = { generated, source, kind: ts.SyntaxKind[node.kind] }
    const buffer = this.captureMappingBuffers[this.captureMappingBuffers.length - 1]
    if (buffer) {
      // Fragment-relative mapping; dedupe within fragment buffer.
      if (
        buffer.some(
          (m) =>
            m.generated.line === generated.line &&
            m.generated.column === generated.column &&
            m.source.line === source.line &&
            m.source.column === source.column,
        )
      )
        return
      buffer.push({ ...entry, __node: node })
      return
    }
    const key = `${generated.line}:${generated.column}:${source.line}:${source.column}`
    if (this.mappingKeys.has(key)) return
    this.mappingKeys.add(key)
    this.mappings.push(entry)
  }

  private emitEntrypointLowering(): void {
    const info = this.defaultClassInfo
    if (!info) return

    if (!info.hasSetup && !info.hasRun) {
      this.report(
        info.node,
        'COS app classes must declare setup(content,width,height) for UI apps or run(args) for console/command apps.',
        'error',
        'cos/entrypoint-contract',
      )
      return
    }

    this.writeLine('')
    if (info.hasSetup) {
      // UI app: runtime launches __cos_entry (class or instance).
      this.writeLine(`var __cos_entry = ${info.name}`)
      return
    }

    // Console/command app: runtime invokes __cos_entry.run(args).
    this.writeLine(`var __cos_entry = ${info.name}()`)
  }
}

export interface BerryCompileOptions {
  /**
   * When true (default), enforce COS entrypoint contract and R13 memory-conscious
   * top-level rules. Set false for module/snippet compilation (tests, libraries).
   */
  entrypoint?: boolean
  /**
   * Compiler profile. `dev` (default) emits debug metadata/mappings; `prod` reduces
   * compiler debug metadata while keeping generated Berry behavior unchanged.
   */
  profile?: BerryCompileProfile
}

/**
 * @brief Generate Berry source from a TypeScript AST.
 * R13: always-on memory-conscious/entrypoint rules fail compilation on violations.
 */
export function generateBerry(sourceFile: ts.SourceFile, options: BerryCompileOptions = {}): BerryGenerationResult {
  const entrypoint = options.entrypoint !== false
  const profile = options.profile ?? 'dev'
  const parseDiagnostics = readParseDiagnostics(sourceFile)
  if (parseDiagnostics.length > 0) {
    const diagnostics = parseDiagnostics.map((diagnostic) =>
      diagnosticAt(sourceFile, diagnostic.start ?? 0, diagnosticText(diagnostic.messageText), 'error'),
    )
    return {
      ok: false,
      code: '',
      diagnostics,
      metadata: {
        profile,
        sourceFileName: sourceFile.fileName || undefined,
        mappings: [],
        notes: ['Parse failed; no Berry output or source mappings were produced.'],
      },
    }
  }

  const session = new GenerationSession(sourceFile, entrypoint, profile)
  enforceTopLevelRules(session, sourceFile, entrypoint)
  for (const statement of sourceFile.statements) {
    if (entrypoint && ts.isClassDeclaration(statement) && hasExportModifier(statement, ts.SyntaxKind.DefaultKeyword)) {
      session.emitTopLevelClass(statement)
      continue
    }
    session.emitStatement(statement)
  }

  return session.toResult()
}

function enforceTopLevelRules(session: GenerationSession, sourceFile: ts.SourceFile, entrypoint: boolean): void {
  if (!entrypoint) return
  let defaultClassCount = 0
  for (const statement of sourceFile.statements) {
    if (ts.isEmptyStatement(statement)) continue
    if (isAmbientStatement(statement)) continue
    if (ts.isInterfaceDeclaration(statement) || ts.isTypeAliasDeclaration(statement)) continue
    if (ts.isImportDeclaration(statement) || ts.isImportEqualsDeclaration(statement)) continue
    if (ts.isExportDeclaration(statement)) continue // validated by emitter; re-exports uncommon in entrypoints

    if (ts.isClassDeclaration(statement)) {
      if (hasExportModifier(statement, ts.SyntaxKind.DefaultKeyword)) {
        defaultClassCount++
        continue
      }
      session.report(
        statement,
        'Top-level classes must be the default exported COS app class.',
        'error',
        'cos/top-level-class',
      )
      continue
    }

    if (ts.isVariableStatement(statement)) {
      reportModuleScopeVariables(session, statement)
      continue
    }

    if (ts.isFunctionDeclaration(statement)) {
      session.report(
        statement,
        'Top-level functions are not allowed in COS entrypoints. Move logic into the default exported app class.',
        'error',
        'cos/no-top-level-function',
      )
      continue
    }

    if (ts.isExportAssignment(statement)) {
      if (statement.isExportEquals) {
        session.report(statement, 'export = is not supported.', 'error', 'cos/export-equals')
        continue
      }
      // export default <expr> - not a class form we support as entrypoint
      session.report(
        statement,
        'COS entrypoints must `export default class ...` (not an expression).',
        'error',
        'cos/default-class-entrypoint',
      )
      continue
    }

    if (ts.isEnumDeclaration(statement)) {
      session.report(statement, 'Enums are not supported yet.', 'error', 'cos/no-enum')
      continue
    }

    // Any other top-level statement is executable/control flow/module side effect.
    session.report(
      statement,
      'Top-level executable statements are not allowed (R13). Move side effects into setup()/run()/methods of the default exported app class.',
      'error',
      'cos/no-top-level-exec',
    )
  }

  if (defaultClassCount === 0) {
    session.report(
      sourceFile,
      'COS entrypoints must declare a single `export default class` extending CosApplication or CosUiApplication.',
      'error',
      'cos/default-class-entrypoint',
    )
  } else if (defaultClassCount > 1) {
    session.report(sourceFile, 'Only one default exported class is supported.', 'error', 'cos/one-default-export')
  }
}

function reportModuleScopeVariables(session: GenerationSession, statement: ts.VariableStatement): void {
  const { flags } = statement.declarationList
  if (flags & ts.NodeFlags.Const) {
    // const bindings: allow? Strict R13 says avoid mutable globals/state; const is not mutable binding.
    // But app state should live in class. Disallow module-scope variables to keep state local.
    // Exception: none needed for current entrypoint model.
  }
  // let/var/const all disallowed at module scope for app state hygiene.
  session.report(
    statement,
    'Module-scope variables are not allowed (R13). Keep state inside the app class instance or local to run/setup/methods.',
    'error',
    'cos/no-module-state',
  )
}

function isAmbientStatement(node: ts.Statement): boolean {
  if (!ts.canHaveModifiers(node)) return false
  const modifiers = ts.getModifiers(node)
  return !!modifiers?.some((m) => m.kind === ts.SyntaxKind.DeclareKeyword)
}

function hasExportModifier(node: ts.Node, kind: ts.SyntaxKind): boolean {
  if (!ts.canHaveModifiers(node)) return false
  return !!ts.getModifiers(node)?.some((m) => m.kind === kind)
}
