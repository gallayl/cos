import ts from '@typescript/typescript6'

import type { GenerationContext } from './context'
import { parameterList } from './parameters'

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
  const params = parameterList(ctx, parameters)
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

/**
 * @brief Lower an arrow or function expression to a Berry `def` expression.
 */
export function emitInlineFunction(ctx: GenerationContext, node: ts.ArrowFunction | ts.FunctionExpression): string {
  if (hasUnsupportedFunctionModifiers(ctx, node)) return 'nil'
  if (ts.isFunctionExpression(node) && node.name) {
    ctx.report(node, 'Named function expressions are not supported.')
    return 'nil'
  }

  ctx.pushScope()
  const params = parameterList(ctx, node.parameters)
  if (params === undefined) {
    ctx.popScope()
    return 'nil'
  }

  const { body } = node
  if (!ts.isBlock(body)) {
    const expression = ctx.emitExpression(body)
    ctx.popScope()
    return `def (${params}) return ${expression} end`
  }

  const captured = ctx.capture(() => {
    ctx.indent()
    for (const statement of body.statements) ctx.emitStatement(statement)
    ctx.dedent()
  })
  ctx.popScope()
  if (captured.length === 0) return `def (${params}) end`
  return `def (${params})\n${captured}\nend`
}
