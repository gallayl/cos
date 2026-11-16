import { execFileSync } from 'node:child_process'
import { mkdirSync, mkdtempSync, readdirSync, readFileSync, rmSync, statSync, writeFileSync } from 'node:fs'
import { tmpdir } from 'node:os'
import { join, relative, sep } from 'node:path'
import { fileURLToPath, pathToFileURL } from 'node:url'

import { astToBerry } from '../src/ide/ast-to-berry'
import { getAst } from '../src/ide/get-ast'
import { collectRelativeImportSpecifiers } from '../src/ide/import-specifiers'

const repoRoot = fileURLToPath(new URL('../..', import.meta.url))
const storageRoot = join(repoRoot, 'storage')
const flashRoot = join(storageRoot, 'flash_data')
const typescriptRoot = join(storageRoot, 'ts')
const configPath = join(storageRoot, 'tsconfig.json')
const typescriptBin = new URL('../node_modules/typescript/bin/', import.meta.url)
const tscPath = (() => {
  for (const candidate of ['tsc', 'tsc6']) {
    const path = fileURLToPath(new URL(candidate, typescriptBin))
    try {
      readFileSync(path)
      return path
    } catch {
      // try next
    }
  }
  return fileURLToPath(new URL('tsc', typescriptBin))
})()
const sdkIndex = fileURLToPath(new URL('../src/cos-sdk/index.d.ts', import.meta.url))

const OPERATORS = ['&&', '||', '==', '!=', '<=', '>=', '+=', '-=', '*=', '/=', '%=']

export interface ScriptCheck {
  /** Path relative to `storage/ts` and `storage/flash_data`, using `/`. */
  name: string
  diagnostics: readonly string[]
  /** Empty when the compiled Berry matches the device script. */
  diff: string
}

export interface StorageCheck {
  optionProblems: readonly string[]
  /** Diagnostics from `storage/tsconfig.json`, SDK included. */
  typecheck: readonly string[]
  /** Diagnostics for a snippet that uses JavaScript globals. */
  nonSdk: readonly string[]
  scripts: readonly ScriptCheck[]
}

export interface StorageRoots {
  typescriptRoot: string
  flashRoot: string
}

export interface DependencyEdge {
  specifier: string
  resolved: string
}

/**
 * Typecheck every storage script against the COS SDK, compile each one to
 * Berry, and compare that output with the device script.
 */
export function verifyStorage(): StorageCheck {
  const config = JSON.parse(readFileSync(configPath, 'utf8')) as {
    compilerOptions?: Record<string, unknown>
  }
  const typecheck = runTsc(configPath)
  return {
    optionProblems: compilerOptionProblems(config.compilerOptions ?? {}),
    typecheck: typecheck.status === 0 ? [] : outputLines(typecheck.output),
    nonSdk: nonSdkDiagnostics(config.compilerOptions ?? {}),
    scripts: checkScripts(),
  }
}

/** Compile/write storage Berry outputs. Dependency-aware and incremental for `--write`. */
export function writeCompiledBerry(roots: StorageRoots = { typescriptRoot, flashRoot }): number {
  try {
    return writeCompiledBerryInner(roots)
  } catch (error) {
    console.error(error instanceof Error ? error.message : String(error))
    return 1
  }
}

function writeCompiledBerryInner(roots: StorageRoots): number {
  const graph = buildRelativeImportGraph(roots.typescriptRoot)
  assertNoRelativeImportIssues(graph)
  let written = 0
  for (const name of [...graph.names].sort()) {
    const typescriptPath = join(roots.typescriptRoot, `${name}.ts`)
    const berryPath = join(roots.flashRoot, `${name}.be`)
    if (!isBerryOutputStale(typescriptPath, berryPath)) continue
    const result = astToBerry(getAst(readFileSync(typescriptPath, 'utf8')))
    if (!result.ok) {
      throw new Error(`${name}: ${result.diagnostics.map((d) => `${d.line}:${d.column}: ${d.message}`).join('; ')}`)
    }
    mkdirSync(join(berryPath, '..'), { recursive: true })
    writeFileSync(berryPath, result.code.endsWith('\n') ? result.code : `${result.code}\n`)
    written += 1
  }
  console.log(`Wrote ${written} Berry files to ${relative(repoRoot, roots.flashRoot) || roots.flashRoot}`)
  return 0
}

export function buildRelativeImportGraph(tsRoot: string): {
  names: string[]
  deps: Map<string, DependencyEdge[]>
  missing: Map<string, DependencyEdge[]>
} {
  const typescript = collect(tsRoot, '.ts')
  const names = [...typescript.keys()].sort()
  const nameSet = new Set(names)
  const deps = new Map<string, DependencyEdge[]>()
  const missing = new Map<string, DependencyEdge[]>()
  for (const name of names) {
    const source = readFileSync(typescript.get(name) ?? join(tsRoot, `${name}.ts`), 'utf8')
    const edges: DependencyEdge[] = []
    const miss: DependencyEdge[] = []
    for (const specifier of collectRelativeImportSpecifiers(source)) {
      const resolved = resolveRelativeStorageImport(name, specifier, tsRoot)
      if (!resolved || !nameSet.has(resolved)) {
        miss.push({ specifier, resolved: resolved ?? specifier })
      } else {
        edges.push({ specifier, resolved })
      }
    }
    deps.set(name, edges)
    if (miss.length > 0) missing.set(name, miss)
  }
  return { names, deps, missing }
}

export function assertNoRelativeImportIssues(graph: ReturnType<typeof buildRelativeImportGraph>): void {
  if (graph.missing.size > 0) {
    const lines = [...graph.missing.entries()]
      .sort(([a], [b]) => a.localeCompare(b))
      .flatMap(([name, edges]) =>
        edges
          .sort((a, b) => a.specifier.localeCompare(b.specifier))
          .map(
            (edge) => `${name}: missing/unresolvable relative import '${edge.specifier}' (resolved '${edge.resolved}')`,
          ),
      )
    throw new Error(lines.join('\n'))
  }
  const cycle = findRelativeImportCycle(graph.deps, graph.names)
  if (cycle) {
    throw new Error(`relative import cycle: ${formatCycle(cycle, graph.deps)}`)
  }
}

export function findRelativeImportCycle(
  deps: Map<string, DependencyEdge[]>,
  names: readonly string[] = [...deps.keys()].sort(),
): string[] | null {
  const state = new Map<string, 0 | 1 | 2>()
  const stack: string[] = []
  for (const name of names) state.set(name, 0)
  const visit = (name: string): string[] | null => {
    state.set(name, 1)
    stack.push(name)
    for (const edge of [...(deps.get(name) ?? [])].sort((a, b) => a.resolved.localeCompare(b.resolved))) {
      const dep = edge.resolved
      const depState = state.get(dep) ?? 0
      if (depState === 1) {
        const idx = stack.indexOf(dep)
        return idx >= 0 ? [...stack.slice(idx), dep] : [dep, name, dep]
      }
      if (depState === 0) {
        const found = visit(dep)
        if (found) return found
      }
    }
    stack.pop()
    state.set(name, 2)
    return null
  }
  for (const name of names) {
    if ((state.get(name) ?? 0) === 0) {
      const found = visit(name)
      if (found) return found
    }
  }
  return null
}

function formatCycle(cycle: readonly string[], deps: Map<string, DependencyEdge[]>): string {
  const parts: string[] = []
  for (let i = 0; i < cycle.length - 1; i += 1) {
    const from = cycle[i] ?? ''
    const to = cycle[i + 1] ?? ''
    const edge = (deps.get(from) ?? []).find((d) => d.resolved === to)
    parts.push(edge ? `${from} --'${edge.specifier}'--> ${to}` : `${from} --> ${to}`)
  }
  return parts.join(' -> ')
}

export function resolveRelativeStorageImport(importerName: string, specifier: string, tsRoot: string): string | null {
  if (!specifier.startsWith('./') && !specifier.startsWith('../')) return null
  const importerDir = importerName.includes('/') ? importerName.slice(0, importerName.lastIndexOf('/')) : ''
  const joined = normalizePosixPath(`${importerDir ? `${importerDir}/` : ''}${specifier}`)
  if (joined === null) return null
  let target = joined
  if (target.endsWith('.ts')) {
    target = target.slice(0, -3)
  } else if (/\.[^/]+$/.test(target)) {
    return null
  }
  const targetPath = join(tsRoot, ...target.split('/'))
  const rootNorm = normalizeFsPath(tsRoot)
  const targetNorm = normalizeFsPath(targetPath)
  if (targetNorm !== rootNorm && !targetNorm.startsWith(`${rootNorm}${sep}`)) return null
  return target
}

function normalizePosixPath(path: string): string | null {
  const parts = path.replace(/\\/g, '/').split('/')
  const stack: string[] = []
  for (const part of parts) {
    if (part === '' || part === '.') continue
    if (part === '..') {
      if (stack.length === 0) return null
      stack.pop()
      continue
    }
    stack.push(part)
  }
  return stack.join('/')
}

function normalizeFsPath(path: string): string {
  return join(path)
}

export function isBerryOutputStale(typescriptPath: string, berryPath: string): boolean {
  // shortcut: --write trusts mtime; a newer-but-bad .be is only caught by verify/check-storage, add content verification if this becomes an issue
  try {
    const berryMtime = statSync(berryPath).mtimeMs
    const tsMtime = statSync(typescriptPath).mtimeMs
    return berryMtime < tsMtime
  } catch {
    return true
  }
}

/** @returns Process exit code: `0` when every check passed. */
export function main(args: readonly string[] = process.argv.slice(2)): number {
  if (args.includes('--write')) {
    return writeCompiledBerry()
  }
  const report = verifyStorage()
  const problems = reportProblems(report)
  if (problems.length === 0) {
    console.log(`COS storage scripts: ${report.scripts.length} TypeScript files compiled to Berry`)
    return 0
  }
  console.error(problems.join('\n'))
  return 1
}

function reportProblems(report: StorageCheck): string[] {
  const problems = [
    ...report.optionProblems,
    ...report.typecheck,
    ...report.scripts.flatMap((script) => [
      ...script.diagnostics.map((message) => `${script.name}: ${message}`),
      ...(script.diff ? [`${script.name}\n${script.diff}`] : []),
    ]),
  ]
  const nonSdk = report.nonSdk.join('\n')
  if (!nonSdk.includes('console') || !nonSdk.includes('setTimeout') || !nonSdk.includes('Math')) {
    problems.push(`COS SDK only check did not reject JavaScript globals:\n${nonSdk}`)
  }
  return problems
}

function compilerOptionProblems(options: Record<string, unknown>): string[] {
  const problems: string[] = []
  if (options.noLib !== true) problems.push('storage/tsconfig.json must set noLib')
  if (options.strict !== true) problems.push('storage/tsconfig.json must set strict')
  if (options.lib !== undefined) problems.push('storage/tsconfig.json must not set lib')
  if (!Array.isArray(options.types) || options.types.length > 0) {
    problems.push('storage/tsconfig.json must set types to an empty list')
  }
  if (options.moduleDetection !== 'force') {
    problems.push('storage/tsconfig.json must force module detection')
  }
  return problems
}

function nonSdkDiagnostics(options: Record<string, unknown>): string[] {
  const dir = mkdtempSync(join(tmpdir(), 'cos-sdk-'))
  try {
    writeFileSync(join(dir, 'guard.ts'), `console.log("nope");\nsetTimeout(() => {}, 1);\nMath.random();\n`)
    writeFileSync(
      join(dir, 'tsconfig.json'),
      JSON.stringify(
        {
          compilerOptions: options,
          files: ['guard.ts', sdkIndex],
        },
        null,
        2,
      ),
    )
    return outputLines(runTsc(join(dir, 'tsconfig.json')).output)
  } finally {
    rmSync(dir, { recursive: true, force: true })
  }
}

function runTsc(project: string): { status: number; output: string } {
  try {
    const output = execFileSync(process.execPath, [tscPath, '-p', project, '--noEmit', '--pretty', 'false'], {
      encoding: 'utf8',
    })
    return { status: 0, output }
  } catch (error) {
    const failed = error as { status?: number; stdout?: string; stderr?: string }
    return { status: failed.status ?? 1, output: `${failed.stdout ?? ''}\n${failed.stderr ?? ''}` }
  }
}

function outputLines(output: string): string[] {
  return output
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line.length > 0)
}

function checkScripts(): ScriptCheck[] {
  const berry = collect(flashRoot, '.be')
  const typescript = collect(typescriptRoot, '.ts')
  const names = [...new Set([...berry.keys(), ...typescript.keys()])]
    .sort()
    .filter((name) => typescript.has(name) || !name.startsWith('tests/'))
  return names.map((name) => checkOne(name, berry.get(name), typescript.get(name)))
}

function checkOne(name: string, berryPath: string | undefined, typescriptPath: string | undefined): ScriptCheck {
  if (!berryPath || !typescriptPath) {
    // Hand-written Berry scripts under tests/ (e.g. R2 profiling/regression)
    // are not compiled from storage TS sources.
    if (!typescriptPath && berryPath && name.startsWith('tests/')) {
      return { name, diagnostics: [], diff: '' }
    }
    const missing = berryPath ? 'TypeScript source' : 'Berry script'
    return { name, diagnostics: [`Missing ${missing}`], diff: '' }
  }
  const result = astToBerry(getAst(readFileSync(typescriptPath, 'utf8')))
  if (!result.ok) {
    return {
      name,
      diagnostics: result.diagnostics.map(
        (diagnostic) => `${diagnostic.line}:${diagnostic.column}: ${diagnostic.message}`,
      ),
      diff: '',
    }
  }
  const expected = semanticTokens(readFileSync(berryPath, 'utf8'))
  const actual = semanticTokens(result.code)
  return { name, diagnostics: [], diff: tokenDiff(expected, actual) }
}

function collect(root: string, suffix: string): Map<string, string> {
  const files = new Map<string, string>()
  for (const file of walk(root)) {
    if (!file.endsWith(suffix)) continue
    const key = relative(root, file.slice(0, -suffix.length)).split(sep).join('/')
    files.set(key, file)
  }
  return files
}

function walk(dir: string): string[] {
  const files: string[] = []
  for (const entry of readdirSync(dir)) {
    const path = join(dir, entry)
    if (statSync(path).isDirectory()) files.push(...walk(path))
    else files.push(path)
  }
  return files
}

/** Tokens of a Berry program, ignoring comments, layout, and `map["key"]` vs `map.key`. */
export function semanticTokens(source: string): string[] {
  return expandUninitializedVars(memberIndexToDot(tokenize(source)))
}

function tokenize(source: string): string[] {
  const tokens: string[] = []
  let index = 0
  while (index < source.length) {
    const char = source[index] ?? ''
    if (char === ' ' || char === '\t' || char === '\r' || char === '\n') {
      index += 1
      continue
    }
    if (char === '#') {
      while (index < source.length && source[index] !== '\n') index += 1
      continue
    }
    if (char === '"' || char === "'") {
      const quote = char
      let end = index + 1
      while (end < source.length) {
        if (source[end] === '\\') {
          end += 2
          continue
        }
        end += 1
        if (source[end - 1] === quote) break
      }
      tokens.push(source.slice(index, end))
      index = end
      continue
    }
    const operator = OPERATORS.find((item) => source.startsWith(item, index))
    if (operator) {
      tokens.push(operator)
      index += operator.length
      continue
    }
    if ('()[]{}.,:+-*/%!<>='.includes(char)) {
      tokens.push(char)
      index += 1
      continue
    }
    if (/[A-Za-z_]/.test(char) || /[0-9]/.test(char)) {
      const pattern = /[A-Za-z_]/.test(char) ? /[A-Za-z0-9_]/ : /[0-9]/
      let end = index + 1
      while (end < source.length && pattern.test(source[end] ?? '')) end += 1
      tokens.push(source.slice(index, end))
      index = end
      continue
    }
    throw new Error(`Unexpected ${JSON.stringify(char)} while tokenizing Berry`)
  }
  return tokens
}

function memberIndexToDot(tokens: readonly string[]): string[] {
  const normalized: string[] = []
  for (let index = 0; index < tokens.length; index += 1) {
    const key = tokens[index + 1]
    if (
      tokens[index] === '[' &&
      tokens[index + 2] === ']' &&
      key !== undefined &&
      /^"(?:[A-Za-z_][A-Za-z0-9_]*)"$/.test(key)
    ) {
      normalized.push('.', key.slice(1, -1))
      index += 2
      continue
    }
    normalized.push(tokens[index] ?? '')
  }
  return normalized
}

function expandUninitializedVars(tokens: readonly string[]): string[] {
  const expanded: string[] = []
  let index = 0
  while (index < tokens.length) {
    const name = tokens[index + 1]
    if (tokens[index] === 'var' && isIdentifier(name)) {
      const names = [name]
      let cursor = index + 2
      while (tokens[cursor] === ',' && isIdentifier(tokens[cursor + 1])) {
        names.push(tokens[cursor + 1] ?? '')
        cursor += 2
      }
      for (const binding of names) expanded.push('var', binding)
      index = cursor
      continue
    }
    expanded.push(tokens[index] ?? '')
    index += 1
  }
  return expanded
}

function isIdentifier(token: string | undefined): token is string {
  return token !== undefined && /^[A-Za-z_][A-Za-z0-9_]*$/.test(token)
}

function tokenDiff(expected: readonly string[], actual: readonly string[]): string {
  const length = Math.max(expected.length, actual.length)
  for (let index = 0; index < length; index += 1) {
    if (expected[index] === actual[index]) continue
    const from = Math.max(0, index - 10)
    const to = Math.min(length, index + 10)
    const mark = (tokens: readonly string[]) =>
      tokens
        .slice(from, to)
        .map((token, offset) => (from + offset === index ? `>>${token ?? '∅'}<<` : (token ?? '∅')))
        .join(' ')
    return [`token ${index}`, `  berry: ${mark(expected)}`, `  ts:    ${mark(actual)}`].join('\n')
  }
  return ''
}

const entry = process.argv[1]
if (entry && import.meta.url === pathToFileURL(entry).href) {
  process.exit(main())
}
