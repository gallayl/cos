import ts from '@typescript/typescript6'

import type { GenerationContext } from './context'
import { emitExpressionWithSpans } from './expression-generator'
import type { ExpressionSpan } from './expression-generator'
import { parameterList, parameterListWithSpans } from './parameters'

export function hasUnsupportedFunctionModifiers(ctx: GenerationContext, node: ts.FunctionLikeDeclarationBase): boolean {
  if (node.asteriskToken) {
    ctx.report(node, 'Generators are not supported.')
    return true
  }
  if (!ts.canHaveModifiers(node)) return false
  const asyncModifier = (ts.getModifiers(node) ?? []).some((modifier) => modifier.kind === ts.SyntaxKind.AsyncKeyword)
  if (asyncModifier) {
    ctx.report(node, 'Async functions are not supported.')
    return true
  }
  return false
}

/**
 * @brief Open a named Berry function in the current writer and its parameter scope.
 * @return False when the signature was rejected. The scope is not left open.
 */
export function beginFunction(
  ctx: GenerationContext,
  header: string,
  parameters: readonly ts.ParameterDeclaration[],
): boolean {
  ctx.pushScope()
  // `header` is the function prefix on the written line, e.g. "def name" -> params start at header.length + 1
  const params = parameterList(ctx, parameters, header.length + 1)
  if (params === undefined) {
    ctx.popScope()
    return false
  }
  ctx.writeLine(`${header}(${params})`)
  ctx.indent()
  return true
}

export function endFunction(ctx: GenerationContext): void {
  ctx.dedent()
  ctx.writeLine('end')
  ctx.popScope()
}

export interface InlineFunctionEmission {
  text: string
  /** Spans relative to start of `text` (offset 0 = first char of returned inline function). */
  spans: ExpressionSpan[]
}

/**
 * @brief Lower an arrow or function expression to a Berry `def` expression.
 * Returns text and internal expression spans (relative to returned string start) for
 * concise-body return expressions and parameter defaults. Block-body inline functions
 * return no internal spans (multiline/captured body mapping is not adjusted here).
 */
export function emitInlineFunction(
  ctx: GenerationContext,
  node: ts.ArrowFunction | ts.FunctionExpression,
): InlineFunctionEmission {
  if (hasUnsupportedFunctionModifiers(ctx, node)) return { text: 'nil', spans: [] }
  if (ts.isFunctionExpression(node) && node.name) {
    ctx.report(node, 'Named function expressions are not supported.')
    return { text: 'nil', spans: [] }
  }

  ctx.pushScope()
  const paramsBase = 'def ('.length
  const paramsResult = parameterListWithSpans(ctx, node.parameters)
  if (paramsResult === undefined) {
    ctx.popScope()
    return { text: 'nil', spans: [] }
  }
  const paramSpans = shiftLocalSpans(paramsResult.spans, paramsBase)

  const { body } = node
  if (!ts.isBlock(body)) {
    const prefix = `def (${paramsResult.text}) return `
    const emitted = emitExpressionWithSpans(ctx, body)
    ctx.popScope()
    return {
      text: `${prefix}${emitted.text} end`,
      spans: [...paramSpans, ...shiftLocalSpans(emitted.spans, prefix.length)],
    }
  }

  const captured = ctx.capture(() => {
    ctx.indent()
    for (const statement of body.statements) ctx.emitStatement(statement)
    ctx.dedent()
  })
  ctx.popScope()
  const blockSpans: ExpressionSpan[] = [...paramSpans]
  // Remap fragment-relative mappings into inline-function-text coordinates:
  // "def (params)\n" is line 0 of inline text, captured content starts at line 1.
  for (const mapping of captured.mappings as unknown as Array<{ generated: { line: number; column: number }; __node?: ts.Node }>) {
    if (!mapping.__node) continue
    blockSpans.push({
      line: mapping.generated.line, // fragment line 1 -> inline text line 1
      column: Math.max(0, mapping.generated.column - 1), // 0-based from line start; writer indent added for line>0
      node: mapping.__node,
    })
  }
  if (captured.text.length === 0) return { text: `def (${paramsResult.text}) end`, spans: paramSpans }
  return { text: `def (${paramsResult.text})\n${captured.text}\nend`, spans: blockSpans }
}

function shiftLocalSpans(spans: readonly ExpressionSpan[], prefix: string | number): ExpressionSpan[] {
  if (typeof prefix === 'number') {
    if (prefix === 0) return [...spans]
    return spans.map((span) => (span.line === 0 ? { ...span, column: span.column + prefix } : { ...span }))
  }
  if (prefix.length === 0) return [...spans]
  const parts = prefix.split('\n')
  if (parts.length === 1) {
    const delta = prefix.length
    return spans.map((span) => (span.line === 0 ? { ...span, column: span.column + delta } : { ...span }))
  }
  const lineDelta = parts.length - 1
  const lastLen = (parts[parts.length - 1] ?? '').length
  return spans.map((span) =>
    span.line === 0
      ? { ...span, line: lineDelta, column: span.column + lastLen }
      : { ...span, line: span.line + lineDelta },
  )
}
