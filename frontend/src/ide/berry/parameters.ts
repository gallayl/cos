import ts from '@typescript/typescript6'

import type { GenerationContext } from './context'
import { emitExpressionMapped, emitExpressionWithSpans, type ExpressionSpan } from './expression-generator'

/**
 * @brief Emit a Berry parameter list and record the names in the current scope.
 * When `baseColumnOffset` is provided (0-based from statement indent/start of the written def line),
 * default-value expressions are recorded with span-aware debug mappings.
 * @return Undefined when a parameter cannot be represented.
 */
export function parameterList(
  ctx: GenerationContext,
  parameters: readonly ts.ParameterDeclaration[],
  baseColumnOffset?: number,
): string | undefined {
  const result = parameterListWithSpansInternal(ctx, parameters, baseColumnOffset, baseColumnOffset !== undefined)
  return result?.text
}

export interface ParameterListEmission {
  text: string
  /** Spans relative to start of parameter list text (inside `def (...)`). */
  spans: ExpressionSpan[]
}

/**
 * @brief Emit parameter list and return default-expression spans relative to list start.
 * Does not record mappings directly; caller decides how to record/shift spans.
 */
export function parameterListWithSpans(
  ctx: GenerationContext,
  parameters: readonly ts.ParameterDeclaration[],
  baseWithinList = 0,
): ParameterListEmission | undefined {
  return parameterListWithSpansInternal(ctx, parameters, baseWithinList, false)
}

function parameterListWithSpansInternal(
  ctx: GenerationContext,
  parameters: readonly ts.ParameterDeclaration[],
  baseOffset: number | undefined,
  recordMappings: boolean,
): ParameterListEmission | undefined {
  const parts: string[] = []
  const spans: ExpressionSpan[] = []
  let offset = 0
  for (const parameter of parameters) {
    if (parameter.dotDotDotToken) {
      ctx.report(parameter, 'Rest parameters are not supported.')
      return undefined
    }
    if (!ts.isIdentifier(parameter.name)) {
      ctx.report(parameter, 'Destructuring is not supported.')
      return undefined
    }
    if (parameter.name.text === 'this') continue
    if (ts.canHaveModifiers(parameter) && (ts.getModifiers(parameter)?.length ?? 0) > 0) {
      ctx.report(parameter, 'Parameter properties are not supported.')
      return undefined
    }
    ctx.declareName(parameter.name.text, parameter.name)
    if (parts.length > 0) offset += ', '.length
    if (parameter.initializer) {
      const prefix = `${parameter.name.text} = `
      const exprOffsetInList = offset + prefix.length
      if (recordMappings && baseOffset !== undefined) {
        const value = emitExpressionMapped(ctx, parameter.initializer, baseOffset + exprOffsetInList)
        const part = `${prefix}${value}`
        parts.push(part)
        offset += part.length
      } else {
        const emitted = emitExpressionWithSpans(ctx, parameter.initializer)
        spans.push(
          ...emitted.spans.map((s) => (s.line === 0 ? { ...s, column: s.column + exprOffsetInList } : { ...s })),
        )
        const part = `${prefix}${emitted.text}`
        parts.push(part)
        offset += part.length
      }
    } else {
      parts.push(parameter.name.text)
      offset += parameter.name.text.length
    }
  }
  return { text: parts.join(', '), spans }
}
