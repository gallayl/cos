import { describe, expect, it } from 'vitest'

import {
  annotateBerryErrorText,
  astToBerry,
  findSourceLocation,
  formatMappedSource,
  mapBerryLineToSource,
} from '../ast-to-berry'
import type { BerryCompileMetadata } from '../ast-to-berry'
import { getAst } from '../get-ast'

describe('berry source map (R14)', () => {
  it('maps generated Berry lines back to TypeScript in dev profile', () => {
    const source = `function add(a: number, b: number): number {\n  return a + b;\n}\n`
    const result = astToBerry(getAst(source), { entrypoint: false, profile: 'dev' })
    expect(result.ok).toBe(true)
    expect(result.metadata?.profile).toBe('dev')
    expect((result.metadata?.mappings.length ?? 0) > 0).toBe(true)

    // First generated line should map near the function declaration.
    const first = findSourceLocation(result.metadata, { line: 1, column: 1 })
    expect(first?.source.line).toBe(1)
    expect(first?.source.column).toBeGreaterThanOrEqual(1)

    expect(mapBerryLineToSource(result.metadata, 1)).toEqual(first?.source)
    expect(formatMappedSource(result.metadata, 1)).toContain(`${first?.source.line}:${first?.source.column}`)
  })

  it('records construct-level mappings for classes, methods, and control structures', () => {
    const source = `
      export default class App implements CosApplication {
        run(): number {
          if (true) {
            for (let i = 0; i < 1; i = i + 1) {
              log(i)
            }
          }
          try {
            log("x")
          } catch (e) {
            log(e)
          }
          return 0
        }
      }
    `
    const result = astToBerry(getAst(source), { profile: 'dev' })
    expect(result.ok).toBe(true)
    const kinds = new Set(result.metadata?.mappings.map((m) => m.kind))
    expect(kinds.has('ClassDeclaration')).toBe(true)
    expect(kinds.has('MethodDeclaration')).toBe(true)
    expect(
      kinds.has('TrueKeyword') ||
        kinds.has('BinaryExpression') ||
        kinds.has('IfStatement') ||
        kinds.has('TryStatement') ||
        kinds.has('Block') ||
        kinds.has('CallExpression') ||
        kinds.has('ReturnStatement'),
    ).toBe(true)
  })

  it('records expression-level mappings for calls, access, and assignments', () => {
    const source = `
      function work(obj: any): void {
        const v = obj.value
        obj.count = obj.count + 1
        obj.run(v)
        return
      }
    `
    const result = astToBerry(getAst(source), { entrypoint: false, profile: 'dev' })
    expect(result.ok).toBe(true)
    const kinds = new Set(result.metadata?.mappings.map((m) => m.kind))
    expect(kinds.has('PropertyAccessExpression')).toBe(true)
    expect(kinds.has('BinaryExpression')).toBe(true)
    // Statement/expression nodes may share source positions (deduped); ensure call-site
    // line/expression mappings are present for generated statement lines.
    const generatedLines = new Set(result.metadata?.mappings.map((m) => m.generated.line))
    expect(generatedLines.has(2)).toBe(true) // const v = obj.value
    expect(generatedLines.has(3)).toBe(true) // assignment
    expect(generatedLines.has(4)).toBe(true) // obj.run(v)
    expect(generatedLines.has(5)).toBe(true) // return
    const line4 = result.metadata?.mappings.filter((m) => m.generated.line === 4) ?? []
    expect(line4.length).toBeGreaterThan(0)
    expect(line4.some((m) => m.source.line === 5)).toBe(true)
  })

  it('records generated-column offsets for prefixed statements and expressions', () => {
    const source = `function f(x: number): number {\n  var y = x + 1\n  if (y > 0) {\n    return y\n  }\n  throw y\n}\n`
    const result = astToBerry(
      getAst(`
        function f(x: number): number {
          const y = x + 1
          if (y > 0) {
            return y
          }
          throw y
        }
      `),
      { entrypoint: false, profile: 'dev' },
    )
    expect(result.ok).toBe(true)
    const byGen = (line: number) => (result.metadata?.mappings ?? []).filter((m) => m.generated.line === line)

    // "    var y = x + 1" -> expression mapped after "    var y = " (indent 4 + 8 chars)
    expect(byGen(2).some((m) => m.generated.column >= 13 && m.source.line === 3)).toBe(true)
    // "    if y > 0" -> condition mapped after "    if "
    expect(byGen(3).some((m) => m.generated.column >= 8 && m.source.line === 4)).toBe(true)
    // "        return y" -> expression mapped after "        return "
    expect(byGen(4).some((m) => m.generated.column >= 16 && m.source.line === 5)).toBe(true)
    // "    raise y" -> expression mapped after "    raise "
    expect(byGen(6).some((m) => m.generated.column >= 11 && m.source.line === 7)).toBe(true)

    // sanity: mapping lookup still resolves line-level locations
    expect(mapBerryLineToSource(result.metadata, 4)?.line).toBeGreaterThan(0)
    void source
  })

  it('records span-aware columns for nested subexpressions (1a prototype)', () => {
    const result = astToBerry(
      getAst(`
        function f(x: number, obj: any): number {
          const y = x + obj.count
          return obj.count
        }
      `),
      { entrypoint: false, profile: 'dev' },
    )
    expect(result.ok).toBe(true)
    const byGen = (line: number) => (result.metadata?.mappings ?? []).filter((m) => m.generated.line === line)
    const line2 = byGen(2)
    const line3 = byGen(3)

    // line 2: "    var y = x + obj.count" (prefix 12 chars => expr starts at col 13)
    // root expression/operand starts at col 13; nested member access starts later in the expression.
    expect(line2.some((m) => m.generated.column === 13)).toBe(true)
    expect(line2.some((m) => m.generated.column > 13 && m.kind === 'PropertyAccessExpression')).toBe(true)

    // line 3: "    return obj.count" (prefix 11 chars => expr starts at col 12)
    expect(line3.some((m) => m.generated.column === 12 && m.kind === 'PropertyAccessExpression')).toBe(true)

    // span tracking should produce multiple expression positions on the same generated line
    expect(new Set(line2.map((m) => m.generated.column)).size).toBeGreaterThan(1)
  })

  it('records span-aware columns for default parameter initializers', () => {
    const result = astToBerry(
      getAst(`
        function f(obj: any, a = obj.count + 1): number {
          return a
        }
      `),
      { entrypoint: false, profile: 'dev' },
    )
    expect(result.ok).toBe(true)
    const line1 = (result.metadata?.mappings ?? []).filter((m) => m.generated.line === 1)
    // "def f(obj, a = obj.count + 1)" -> default expr starts after "def f(obj, a = "
    const base = 'def f(obj, a = '.length + 1 // 1-based
    expect(line1.some((m) => m.generated.column >= base && m.kind === 'BinaryExpression')).toBe(true)
    expect(line1.some((m) => m.generated.column > base)).toBe(true)
  })

  it('records span-aware columns inside concise inline function expressions', () => {
    const result = astToBerry(
      getAst(`
        function outer(obj: any): any {
          const f = (x: any) => x + obj.count
          return f
        }
      `),
      { entrypoint: false, profile: 'dev' },
    )
    expect(result.ok).toBe(true)
    const line2 = (result.metadata?.mappings ?? []).filter((m) => m.generated.line === 2)
    // "    var f = def (x) return x + obj.count end"
    const exprBase = '    var f = '.length + 1 // start of inline def expression (1-based)
    const returnExprBase = '    var f = def (x) return '.length + 1
    expect(line2.some((m) => m.generated.column === exprBase && m.kind === 'ArrowFunction')).toBe(true)
    expect(line2.some((m) => m.generated.column >= returnExprBase && m.kind === 'BinaryExpression')).toBe(true)
    expect(line2.some((m) => m.generated.column > returnExprBase && m.kind === 'PropertyAccessExpression')).toBe(true)
  })

  it('records span-aware columns for concise inline function parameter defaults', () => {
    const result = astToBerry(
      getAst(`
        function outer(obj: any): any {
          const f = (a = obj.count + 1) => a
          return f
        }
      `),
      { entrypoint: false, profile: 'dev' },
    )
    expect(result.ok).toBe(true)
    const line2 = (result.metadata?.mappings ?? []).filter((m) => m.generated.line === 2)
    // "    var f = def (a = obj.count + 1) return a end"
    const defaultExprBase = '    var f = def (a = '.length + 1
    expect(line2.some((m) => m.generated.column >= defaultExprBase && m.kind === 'BinaryExpression')).toBe(true)
    expect(line2.some((m) => m.generated.column > defaultExprBase)).toBe(true)
  })

  it('remaps block-body inline function statement mappings to final generated positions', () => {
    const result = astToBerry(
      getAst(`
        function outer(x: number): any {
          const f = (y: number) => {
            const z = y + x
            return z
          }
          return f
        }
      `),
      { entrypoint: false, profile: 'dev' },
    )
    expect(result.ok).toBe(true)
    const mappings = result.metadata?.mappings ?? []
    const lineCount = result.code.split('\n').length
    expect(mappings.every((m) => m.generated.line >= 1 && m.generated.line <= lineCount)).toBe(true)
    // Statements inside the inline block body should be mapped to actual generated lines (not fragment-relative).
    expect(mappings.some((m) => m.generated.line > 2 && m.source.line >= 4 && m.source.line <= 6)).toBe(true)
  })

  it('remaps nested block-body inline function mappings (multiline capture)', () => {
    const result = astToBerry(
      getAst(`
        function outer(x: number): any {
          const f = () => {
            const g = (y: number) => {
              return y + x
            }
            return g
          }
          return f
        }
      `),
      { entrypoint: false, profile: 'dev' },
    )
    expect(result.ok).toBe(true)
    const mappings = result.metadata?.mappings ?? []
    const lineCount = result.code.split('\n').length
    expect(mappings.every((m) => m.generated.line >= 1 && m.generated.line <= lineCount)).toBe(true)
    // Inner and outer block statements should have mappings placed within generated output.
    expect(mappings.some((m) => m.source.line >= 4 && m.source.line <= 8 && m.generated.line > 2)).toBe(true)
  })

  it('records span-aware columns for nested calls, element access, and new expressions', () => {
    const result = astToBerry(
      getAst(`
        function outer(obj: any, i: number, C: any): any {
          obj.get(i + 1).run(obj.value)
          const x = obj.arr[i + 1]
          const y = new C(obj.x, i + 2)
          return y
        }
      `),
      { entrypoint: false, profile: 'dev' },
    )
    expect(result.ok).toBe(true)
    const byGen = (line: number) => (result.metadata?.mappings ?? []).filter((m) => m.generated.line === line)

    // "    obj.get(i + 1).run(obj.value)" -> multiple nested spans on line 2
    const line2 = byGen(2)
    expect(line2.some((m) => m.kind === 'PropertyAccessExpression')).toBe(true)
    expect(new Set(line2.map((m) => m.generated.column)).size).toBeGreaterThan(2)

    // "    var x = obj.arr[i + 1]" -> element access and binary index
    const line3 = byGen(3)
    expect(line3.some((m) => m.kind === 'ElementAccessExpression')).toBe(true)
    expect(line3.some((m) => m.kind === 'BinaryExpression' && m.generated.column > '    var x = '.length + 1)).toBe(
      true,
    )

    // "    var y = C(obj.x, i + 2)" -> new/call + argument spans
    const line4 = byGen(4)
    expect(
      line4.some((m) => m.kind === 'NewExpression' || m.kind === 'CallExpression' || m.kind === 'Identifier'),
    ).toBe(true)
    expect(line4.some((m) => m.kind === 'PropertyAccessExpression')).toBe(true)
  })

  it('annotates Berry/runtime-style error text with mapped TypeScript locations', () => {
    const source = `function boom(): void {\n  throw "x"\n}\n`
    const result = astToBerry(getAst(source), { entrypoint: false, profile: 'dev' })
    expect(result.ok).toBe(true)
    const annotated = annotateBerryErrorText('syntax_error: /virtual/app.be:1: something', result.metadata)
    expect(annotated).toContain('ts ')
    expect(annotated).toContain('berry line 1')
    expect(annotateBerryErrorText('no line info', result.metadata)).toBe('no line info')
  })

  it('annotates common Berry/runtime error text formats (synthetic)', () => {
    const metadata: BerryCompileMetadata = {
      profile: 'dev',
      sourceFileName: 'app.ts',
      mappings: [
        { generated: { line: 12, column: 1 }, source: { line: 34, column: 5 }, kind: 'CallExpression' },
        { generated: { line: 3, column: 1 }, source: { line: 7, column: 2 }, kind: 'Identifier' },
      ],
      notes: [],
    }
    expect(annotateBerryErrorText('syntax_error: app.be:12:3: unexpected symbol', metadata)).toContain(
      '(ts app.ts:34:5, berry line 12)',
    )
    expect(annotateBerryErrorText('runtime error at line 12', metadata)).toContain('(ts app.ts:34:5, berry line 12)')
    expect(annotateBerryErrorText('Exception in app.be line 12, column 3', metadata)).toContain(
      '(ts app.ts:34:5, berry line 12)',
    )
    expect(annotateBerryErrorText('stack: /virtual/app.be:3:1', metadata)).toContain('(ts app.ts:7:2, berry line 3)')
    // No detectable line -> unchanged
    expect(annotateBerryErrorText('generic failure without location', metadata)).toBe(
      'generic failure without location',
    )
    // Already annotated -> idempotent
    const once = annotateBerryErrorText('app.be:12:1: oops', metadata)
    expect(annotateBerryErrorText(once, metadata)).toBe(once)
  })

  it('findSourceLocation picks the latest mapping at or before a generated column even when mappings are unordered', () => {
    const metadata: BerryCompileMetadata = {
      profile: 'dev',
      mappings: [
        { generated: { line: 2, column: 20 }, source: { line: 10, column: 1 }, kind: 'A' },
        { generated: { line: 2, column: 5 }, source: { line: 11, column: 1 }, kind: 'B' },
        { generated: { line: 2, column: 12 }, source: { line: 12, column: 1 }, kind: 'C' },
      ],
      notes: [],
    }
    expect(findSourceLocation(metadata, { line: 2, column: 15 })?.source.line).toBe(12)
    expect(findSourceLocation(metadata, { line: 2, column: 12 })?.source.line).toBe(12)
    expect(findSourceLocation(metadata, { line: 2, column: 7 })?.source.line).toBe(11)
    expect(findSourceLocation(metadata, { line: 2, column: 4 })).toBeUndefined()
  })

  it('omits detailed mappings in prod profile but keeps metadata', () => {
    const result = astToBerry(getAst(`const x = 1;\n`), { entrypoint: false, profile: 'prod' })
    expect(result.ok).toBe(true)
    expect(result.code).toBe('var x = 1\n')
    expect(result.metadata?.profile).toBe('prod')
    expect(result.metadata?.mappings).toEqual([])
    expect(result.metadata?.notes.length).toBeGreaterThan(0)
    expect(mapBerryLineToSource(result.metadata, 1)).toBeUndefined()
  })

  it('returns metadata for parse failures', () => {
    const result = astToBerry(getAst('const value ='), { entrypoint: false, profile: 'dev' })
    expect(result.ok).toBe(false)
    expect(result.code).toBe('')
    expect(result.metadata?.profile).toBe('dev')
    expect(result.metadata?.mappings).toEqual([])
  })
})
