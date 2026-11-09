import ts from '@typescript/typescript6'

import type { GenerationContext } from './context'

/**
 * @brief Emit a Berry parameter list and record the names in the current scope.
 * @return Undefined when a parameter cannot be represented.
 */
export function parameterList(
  ctx: GenerationContext,
  parameters: readonly ts.ParameterDeclaration[],
): string | undefined {
  const parts: string[] = []
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
    if (parameter.initializer) {
      parts.push(`${parameter.name.text} = ${ctx.emitExpression(parameter.initializer)}`)
    } else {
      parts.push(parameter.name.text)
    }
  }
  return parts.join(', ')
}
