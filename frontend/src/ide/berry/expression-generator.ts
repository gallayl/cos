import ts from '@typescript/typescript6'

import { BOOLEAN_OPERAND_MESSAGE, isStructurallyBoolean, unwrapTransparent } from './boolean-expression'
import type { GenerationContext } from './context'
import { emitInlineFunction } from './function-emitter'
import { isIncrement } from './syntax'

const PREC_ASSIGN = 10
const PREC_OR = 20
const PREC_AND = 30
const PREC_EQUALITY = 40
const PREC_RELATIONAL = 50
const PREC_ADDITIVE = 60
const PREC_MULTIPLICATIVE = 70
const PREC_UNARY = 80
/** Calls, member access, and index access. */
export const CALL_PRECEDENCE = 90
const PREC_ATOM = 100

interface Emitted {
  text: string
  precedence: number
}

interface BinaryOperator {
  berry: string
  precedence: number
  associativity: 'left' | 'right'
}

const BINARY_OPERATORS: Partial<Record<ts.SyntaxKind, BinaryOperator>> = {
  [ts.SyntaxKind.EqualsToken]: { berry: '=', precedence: PREC_ASSIGN, associativity: 'right' },
  [ts.SyntaxKind.PlusEqualsToken]: { berry: '+=', precedence: PREC_ASSIGN, associativity: 'right' },
  [ts.SyntaxKind.MinusEqualsToken]: { berry: '-=', precedence: PREC_ASSIGN, associativity: 'right' },
  [ts.SyntaxKind.AsteriskEqualsToken]: { berry: '*=', precedence: PREC_ASSIGN, associativity: 'right' },
  [ts.SyntaxKind.SlashEqualsToken]: { berry: '/=', precedence: PREC_ASSIGN, associativity: 'right' },
  [ts.SyntaxKind.PercentEqualsToken]: { berry: '%=', precedence: PREC_ASSIGN, associativity: 'right' },
  [ts.SyntaxKind.BarBarToken]: { berry: '||', precedence: PREC_OR, associativity: 'left' },
  [ts.SyntaxKind.AmpersandAmpersandToken]: { berry: '&&', precedence: PREC_AND, associativity: 'left' },
  [ts.SyntaxKind.EqualsEqualsEqualsToken]: { berry: '==', precedence: PREC_EQUALITY, associativity: 'left' },
  [ts.SyntaxKind.ExclamationEqualsEqualsToken]: { berry: '!=', precedence: PREC_EQUALITY, associativity: 'left' },
  [ts.SyntaxKind.LessThanToken]: { berry: '<', precedence: PREC_RELATIONAL, associativity: 'left' },
  [ts.SyntaxKind.GreaterThanToken]: { berry: '>', precedence: PREC_RELATIONAL, associativity: 'left' },
  [ts.SyntaxKind.LessThanEqualsToken]: { berry: '<=', precedence: PREC_RELATIONAL, associativity: 'left' },
  [ts.SyntaxKind.GreaterThanEqualsToken]: { berry: '>=', precedence: PREC_RELATIONAL, associativity: 'left' },
  [ts.SyntaxKind.PlusToken]: { berry: '+', precedence: PREC_ADDITIVE, associativity: 'left' },
  [ts.SyntaxKind.MinusToken]: { berry: '-', precedence: PREC_ADDITIVE, associativity: 'left' },
  [ts.SyntaxKind.AsteriskToken]: { berry: '*', precedence: PREC_MULTIPLICATIVE, associativity: 'left' },
  [ts.SyntaxKind.SlashToken]: { berry: '/', precedence: PREC_MULTIPLICATIVE, associativity: 'left' },
  [ts.SyntaxKind.PercentToken]: { berry: '%', precedence: PREC_MULTIPLICATIVE, associativity: 'left' },
}

/**
 * @brief Quote a Berry string using the cooked TypeScript text.
 */
export function berryString(value: string): string {
  const escaped = value
    .replaceAll('\\', '\\\\')
    .replaceAll('"', '\\"')
    .replaceAll('\n', '\\n')
    .replaceAll('\r', '\\r')
    .replaceAll('\t', '\\t')
  return `"${escaped}"`
}

/**
 * @brief Lower an expression to Berry text safe to place in statement position.
 */
export function emitExpression(ctx: GenerationContext, node: ts.Expression): string {
  return lowerExpression(ctx, node).text
}

/**
 * @brief Like {@link emitExpression}, but parenthesizes values looser than `parentPrecedence`.
 */
export function emitGrouped(ctx: GenerationContext, node: ts.Expression, parentPrecedence: number): string {
  const emitted = lowerExpression(ctx, node)
  return parenthesize(emitted, parentPrecedence, 'left', 'left')
}

function lowerExpression(ctx: GenerationContext, node: ts.Expression): Emitted {
  const unwrapped = unwrapAssertion(node)
  if (unwrapped !== node && !ts.isParenthesizedExpression(node)) {
    return lowerExpression(ctx, unwrapped)
  }

  if (ts.isParenthesizedExpression(node)) {
    const inner = lowerExpression(ctx, node.expression)
    return { text: `(${inner.text})`, precedence: PREC_ATOM }
  }
  if (node.kind === ts.SyntaxKind.TrueKeyword) return atom('true')
  if (node.kind === ts.SyntaxKind.FalseKeyword) return atom('false')
  if (node.kind === ts.SyntaxKind.NullKeyword) return atom('nil')
  if (node.kind === ts.SyntaxKind.ThisKeyword) return atom('self')

  if (ts.isIdentifier(node)) {
    if (node.text === 'undefined' && !ctx.isDeclared('undefined')) return atom('nil')
    return atom(node.text)
  }
  if (ts.isNumericLiteral(node)) return atom(numericLiteralText(node))
  if (ts.isStringLiteral(node)) return atom(berryString(node.text))
  if (ts.isNoSubstitutionTemplateLiteral(node)) return atom(berryString(node.text))
  if (ts.isTemplateExpression(node)) return emitTemplate(ctx, node)
  if (ts.isArrowFunction(node) || ts.isFunctionExpression(node)) {
    return { text: emitInlineFunction(ctx, node), precedence: PREC_ATOM }
  }
  if (ts.isPrefixUnaryExpression(node)) return emitPrefix(ctx, node)
  if (ts.isBinaryExpression(node)) return emitBinary(ctx, node)
  if (ts.isCallExpression(node)) return emitCall(ctx, node)
  if (ts.isNewExpression(node)) return emitNew(ctx, node)
  if (ts.isPropertyAccessExpression(node)) return emitProperty(ctx, node)
  if (ts.isElementAccessExpression(node)) return emitElement(ctx, node)
  if (ts.isArrayLiteralExpression(node)) return emitArray(ctx, node)
  if (ts.isObjectLiteralExpression(node)) return emitObject(ctx, node)

  if (isIncrement(node)) {
    ctx.report(node, 'Increment expressions are only supported as statements.')
    return atom('nil')
  }
  if (ts.isAwaitExpression(node)) {
    ctx.report(node, 'Await is not supported.')
    return atom('nil')
  }
  if (ts.isConditionalExpression(node)) {
    ctx.report(node, 'Conditional expressions are not supported.')
    return atom('nil')
  }
  if (ts.isTypeOfExpression(node)) {
    ctx.report(node, 'typeof is not supported. Use Berry type() or explicit checks.')
    return atom('nil')
  }
  if (ts.isVoidExpression(node)) {
    ctx.report(node, 'The void operator is not supported.')
    return atom('nil')
  }
  if (ts.isDeleteExpression(node)) {
    ctx.report(node, 'The delete operator is not supported.')
    return atom('nil')
  }
  if (node.kind === ts.SyntaxKind.SuperKeyword) {
    ctx.report(node, 'super is not supported.')
    return atom('nil')
  }

  ctx.report(node, `Unsupported syntax: ${ts.SyntaxKind[node.kind]}.`)
  return atom('nil')
}

function atom(text: string): Emitted {
  return { text, precedence: PREC_ATOM }
}

function unwrapAssertion(node: ts.Expression): ts.Expression {
  if (
    ts.isAsExpression(node) ||
    ts.isTypeAssertionExpression(node) ||
    ts.isSatisfiesExpression(node) ||
    ts.isNonNullExpression(node)
  ) {
    return node.expression
  }
  return node
}

function parenthesize(
  operand: Emitted,
  parentPrecedence: number,
  side: 'left' | 'right',
  associativity: 'left' | 'right',
): string {
  const looser = operand.precedence < parentPrecedence
  const sameSideNeedsParens =
    operand.precedence === parentPrecedence &&
    ((associativity === 'left' && side === 'right') || (associativity === 'right' && side === 'left'))
  if (!looser && !sameSideNeedsParens) return operand.text
  return `(${operand.text})`
}

function rejectFunctionOperand(ctx: GenerationContext, node: ts.Expression): void {
  const expression = unwrapTransparent(node)
  if (ts.isArrowFunction(expression) || ts.isFunctionExpression(expression)) {
    ctx.report(node, 'Functions inside operators are not supported.')
  }
}

function emitPrefix(ctx: GenerationContext, node: ts.PrefixUnaryExpression): Emitted {
  if (node.operator === ts.SyntaxKind.ExclamationToken) {
    if (!isStructurallyBoolean(node.operand)) ctx.report(node.operand, BOOLEAN_OPERAND_MESSAGE)
  } else if (node.operator === ts.SyntaxKind.PlusToken) {
    ctx.report(node, 'Unary + is not supported. Use int(value) or real(value) to make numeric intent explicit.')
    return atom('nil')
  } else if (node.operator === ts.SyntaxKind.TildeToken) {
    ctx.report(node, 'Bitwise operators are not supported.')
    return atom('nil')
  } else if (node.operator !== ts.SyntaxKind.MinusToken) {
    ctx.report(node, `Unsupported syntax: ${ts.SyntaxKind[node.operator]}.`)
    return atom('nil')
  }
  rejectFunctionOperand(ctx, node.operand)
  const berry = node.operator === ts.SyntaxKind.ExclamationToken ? '!' : '-'
  const operand = lowerExpression(ctx, node.operand)
  return {
    text: `${berry}${parenthesize(operand, PREC_UNARY, 'right', 'right')}`,
    precedence: PREC_UNARY,
  }
}

function emitBinary(ctx: GenerationContext, node: ts.BinaryExpression): Emitted {
  const { kind } = node.operatorToken
  const rejected = rejectedBinary(kind)
  if (rejected) {
    ctx.report(node.operatorToken, rejected)
    return atom('nil')
  }
  const operator = BINARY_OPERATORS[kind]
  if (!operator) {
    ctx.report(node.operatorToken, `Unsupported syntax: ${ts.SyntaxKind[kind]}.`)
    return atom('nil')
  }
  if (
    (kind === ts.SyntaxKind.AmpersandAmpersandToken || kind === ts.SyntaxKind.BarBarToken) &&
    !isStructurallyBoolean(node.left)
  ) {
    ctx.report(node.left, BOOLEAN_OPERAND_MESSAGE)
  }
  if (kind === ts.SyntaxKind.SlashToken && isIntegerNumericLiteral(node.left) && isIntegerNumericLiteral(node.right)) {
    ctx.report(
      node,
      'Integer literal division uses Berry integer division (1 / 2 is 0). Use real literals (1.0 / 2.0) or real(...) for TS/JS-style division.',
    )
  }
  rejectFunctionOperand(ctx, node.left)
  rejectFunctionOperand(ctx, node.right)
  const left = lowerExpression(ctx, node.left)
  const right = lowerExpression(ctx, node.right)
  return {
    text: `${parenthesize(left, operator.precedence, 'left', operator.associativity)} ${operator.berry} ${parenthesize(right, operator.precedence, 'right', operator.associativity)}`,
    precedence: operator.precedence,
  }
}

function numericLiteralText(node: ts.NumericLiteral): string {
  return node.getText().replaceAll('_', '')
}

function isIntegerNumericLiteral(node: ts.Expression): boolean {
  const expression = unwrapTransparent(node)
  if (!ts.isNumericLiteral(expression)) return false
  // Use source text: TS normalizes `2.0` -> text `2` and `1e3` -> `1000`, but
  // Berry distinguishes int/real by lexical form (`.` / exponent => real).
  const text = numericLiteralText(expression).trim()
  if (
    text.startsWith('0x') ||
    text.startsWith('0X') ||
    text.startsWith('0b') ||
    text.startsWith('0B') ||
    text.startsWith('0o') ||
    text.startsWith('0O')
  ) {
    return true
  }
  return !text.includes('.') && !text.includes('e') && !text.includes('E')
}

function rejectedBinary(kind: ts.SyntaxKind): string | undefined {
  switch (kind) {
    case ts.SyntaxKind.EqualsEqualsToken:
    case ts.SyntaxKind.ExclamationEqualsToken:
      return 'Loose equality is not supported. Use === and !==.'
    case ts.SyntaxKind.QuestionQuestionToken:
      return 'Nullish coalescing is not supported.'
    case ts.SyntaxKind.InstanceOfKeyword:
      return 'instanceof is not supported.'
    case ts.SyntaxKind.InKeyword:
      return 'The in operator is not supported.'
    case ts.SyntaxKind.AsteriskAsteriskToken:
    case ts.SyntaxKind.AsteriskAsteriskEqualsToken:
      return 'Exponentiation is not supported.'
    case ts.SyntaxKind.CommaToken:
      return 'Comma expressions are not supported.'
    case ts.SyntaxKind.AmpersandToken:
    case ts.SyntaxKind.BarToken:
    case ts.SyntaxKind.CaretToken:
    case ts.SyntaxKind.TildeToken:
    case ts.SyntaxKind.LessThanLessThanToken:
    case ts.SyntaxKind.GreaterThanGreaterThanToken:
    case ts.SyntaxKind.GreaterThanGreaterThanGreaterThanToken:
    case ts.SyntaxKind.AmpersandEqualsToken:
    case ts.SyntaxKind.BarEqualsToken:
    case ts.SyntaxKind.CaretEqualsToken:
    case ts.SyntaxKind.LessThanLessThanEqualsToken:
    case ts.SyntaxKind.GreaterThanGreaterThanEqualsToken:
    case ts.SyntaxKind.GreaterThanGreaterThanGreaterThanEqualsToken:
      return 'Bitwise operators are not supported.'
    default:
      return undefined
  }
}

function emitArgument(ctx: GenerationContext, node: ts.Expression): string {
  if (ts.isSpreadElement(node)) {
    ctx.report(node, 'Spread syntax is not supported.')
    return 'nil'
  }
  return lowerExpression(ctx, node).text
}

function emitCall(ctx: GenerationContext, node: ts.CallExpression): Emitted {
  if (node.questionDotToken) ctx.report(node, 'Optional chaining is not supported.')
  const callee = emitGrouped(ctx, node.expression, CALL_PRECEDENCE)
  const args = node.arguments.map((argument) => emitArgument(ctx, argument))
  return { text: `${callee}(${args.join(', ')})`, precedence: CALL_PRECEDENCE }
}

function emitNew(ctx: GenerationContext, node: ts.NewExpression): Emitted {
  const callee = emitGrouped(ctx, node.expression, CALL_PRECEDENCE)
  const args = (node.arguments ?? []).map((argument) => emitArgument(ctx, argument))
  return { text: `${callee}(${args.join(', ')})`, precedence: CALL_PRECEDENCE }
}

function emitProperty(ctx: GenerationContext, node: ts.PropertyAccessExpression): Emitted {
  if (node.questionDotToken) ctx.report(node, 'Optional chaining is not supported.')
  if (ts.isPrivateIdentifier(node.name)) {
    ctx.report(node.name, 'Private identifiers are not supported.')
    return atom('nil')
  }
  const objectText = emitGrouped(ctx, node.expression, CALL_PRECEDENCE)
  const called = isCallee(node)
  if (node.name.text === 'length' && !called) {
    // Berry global size() handles strings (byte length) and objects exposing size().
    // `obj.size()` would fail for plain Berry strings, so prefer `size(obj)`.
    return { text: `size(${objectText})`, precedence: CALL_PRECEDENCE }
  }
  return { text: `${objectText}.${node.name.text}`, precedence: CALL_PRECEDENCE }
}

function emitElement(ctx: GenerationContext, node: ts.ElementAccessExpression): Emitted {
  if (node.questionDotToken) ctx.report(node, 'Optional chaining is not supported.')
  const objectText = emitGrouped(ctx, node.expression, CALL_PRECEDENCE)
  const index = lowerExpression(ctx, node.argumentExpression).text
  return { text: `${objectText}[${index}]`, precedence: CALL_PRECEDENCE }
}

function isCallee(node: ts.Node): boolean {
  const { parent } = node
  return !!parent && ts.isCallExpression(parent) && parent.expression === node
}

function emitArray(ctx: GenerationContext, node: ts.ArrayLiteralExpression): Emitted {
  const elements: string[] = []
  for (const element of node.elements) {
    if (ts.isOmittedExpression(element)) {
      ctx.report(element, 'Array holes are not supported.')
      continue
    }
    if (ts.isSpreadElement(element)) {
      ctx.report(element, 'Spread syntax is not supported.')
      continue
    }
    elements.push(lowerExpression(ctx, element).text)
  }
  return atom(`[${elements.join(', ')}]`)
}

function emitObject(ctx: GenerationContext, node: ts.ObjectLiteralExpression): Emitted {
  const fields: string[] = []
  for (const property of node.properties) {
    if (!ts.isPropertyAssignment(property)) {
      ctx.report(property, 'Only explicit object fields are supported.')
      continue
    }
    const key = objectKey(ctx, property.name)
    if (!key) continue
    fields.push(`${key}: ${lowerExpression(ctx, property.initializer).text}`)
  }
  return atom(`{${fields.join(', ')}}`)
}

function objectKey(ctx: GenerationContext, name: ts.PropertyName): string | undefined {
  if (ts.isIdentifier(name) || ts.isStringLiteral(name) || ts.isNumericLiteral(name)) {
    return berryString(name.text)
  }
  ctx.report(name, 'Computed property names are not supported.')
  return undefined
}

function emitTemplate(ctx: GenerationContext, node: ts.TemplateExpression): Emitted {
  const parts: Emitted[] = []
  if (node.head.text.length > 0) parts.push(atom(berryString(node.head.text)))
  for (const span of node.templateSpans) {
    const value = lowerExpression(ctx, span.expression).text
    parts.push({ text: `str(${value})`, precedence: CALL_PRECEDENCE })
    if (span.literal.text.length > 0) parts.push(atom(berryString(span.literal.text)))
  }
  if (parts.length === 0) return atom('""')
  let current = parts[0] ?? atom('""')
  for (const part of parts.slice(1)) {
    current = {
      text: `${parenthesize(current, PREC_ADDITIVE, 'left', 'left')} + ${parenthesize(part, PREC_ADDITIVE, 'right', 'left')}`,
      precedence: PREC_ADDITIVE,
    }
  }
  return current
}
