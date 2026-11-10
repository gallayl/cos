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

/** Span-aware debug mapping: node starts at (line, column) within emitted expression text (0-based). */
export interface ExpressionSpan {
  /** 0-based line delta within the emitted expression text. */
  line: number
  /** 0-based column within that line of the emitted expression text. */
  column: number
  node: ts.Node
}

export interface EmittedExpression {
  text: string
  precedence: number
  spans: readonly ExpressionSpan[]
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
 * @brief Lower an expression with span-aware debug mapping offsets.
 */
export function emitExpressionWithSpans(ctx: GenerationContext, node: ts.Expression): EmittedExpression {
  return lowerExpression(ctx, node)
}

/**
 * @brief Record expression subexpression mappings at a known generated line/column base.
 * `baseColumnOffset` is 0-based from statement/writer indent start and applies to expression line 0.
 * For multiline expressions, subsequent lines are mapped relative to writer indent (no statement prefix).
 */
export function noteExpressionSpans(
  ctx: GenerationContext,
  baseColumnOffset: number,
  spans: readonly ExpressionSpan[],
): void {
  for (const span of spans) {
    if (span.line === 0) {
      ctx.noteMapping(span.node, baseColumnOffset + span.column)
    } else {
      ctx.noteMappingAt(span.node, span.line, span.column)
    }
  }
}

/**
 * @brief Emit an expression and record its span mappings at `baseColumnOffset`.
 */
export function emitExpressionMapped(
  ctx: GenerationContext,
  node: ts.Expression,
  baseColumnOffset: number,
): string {
  const emitted = lowerExpression(ctx, node)
  noteExpressionSpans(ctx, baseColumnOffset, emitted.spans)
  return emitted.text
}

/**
 * @brief Like {@link emitExpression}, but parenthesizes values looser than `parentPrecedence`.
 * Returns text for compatibility; spans are not exposed (use grouped-with-spans helpers internally).
 */
export function emitGrouped(ctx: GenerationContext, node: ts.Expression, parentPrecedence: number): string {
  const emitted = lowerExpression(ctx, node)
  return parenthesize(emitted, parentPrecedence, 'left', 'left').text
}

/**
 * @brief Emit grouped expression and record span mappings at `baseColumnOffset`.
 */
export function emitGroupedMapped(
  ctx: GenerationContext,
  node: ts.Expression,
  parentPrecedence: number,
  baseColumnOffset: number,
): string {
  const emitted = lowerExpression(ctx, node)
  const grouped = parenthesize(emitted, parentPrecedence, 'left', 'left')
  noteExpressionSpans(ctx, baseColumnOffset, grouped.spans)
  return grouped.text
}

function lowerExpression(ctx: GenerationContext, node: ts.Expression): EmittedExpression {
  const unwrapped = unwrapAssertion(node)
  if (unwrapped !== node && !ts.isParenthesizedExpression(node)) {
    return lowerExpression(ctx, unwrapped)
  }

  if (ts.isParenthesizedExpression(node)) {
    const inner = lowerExpression(ctx, node.expression)
    return wrapParens(inner)
  }
  if (node.kind === ts.SyntaxKind.TrueKeyword) return atom('true', node)
  if (node.kind === ts.SyntaxKind.FalseKeyword) return atom('false', node)
  if (node.kind === ts.SyntaxKind.NullKeyword) return atom('nil', node)
  if (node.kind === ts.SyntaxKind.ThisKeyword) return atom('self', node)

  if (ts.isIdentifier(node)) {
    if (node.text === 'undefined' && !ctx.isDeclared('undefined')) return atom('nil', node)
    return atom(node.text, node)
  }
  if (ts.isNumericLiteral(node)) return atom(numericLiteralText(node), node)
  if (ts.isStringLiteral(node)) return atom(berryString(node.text), node)
  if (ts.isNoSubstitutionTemplateLiteral(node)) return atom(berryString(node.text), node)
  if (ts.isTemplateExpression(node)) return emitTemplate(ctx, node)
  if (ts.isArrowFunction(node) || ts.isFunctionExpression(node)) {
    const inline = emitInlineFunction(ctx, node)
    return { text: inline.text, precedence: PREC_ATOM, spans: [{ line: 0, column: 0, node }, ...inline.spans] }
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

function atom(text: string, node?: ts.Node): EmittedExpression {
  return { text, precedence: PREC_ATOM, spans: node ? [{ line: 0, column: 0, node }] : [] }
}

function shiftSpans(spans: readonly ExpressionSpan[], prefix: string | number): ExpressionSpan[] {
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

function withNode(node: ts.Node, emitted: EmittedExpression): EmittedExpression {
  return { ...emitted, spans: [{ line: 0, column: 0, node }, ...emitted.spans] }
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

interface Parenthesized {
  text: string
  spans: ExpressionSpan[]
}

function parenthesize(
  operand: EmittedExpression,
  parentPrecedence: number,
  side: 'left' | 'right',
  associativity: 'left' | 'right',
): Parenthesized {
  const looser = operand.precedence < parentPrecedence
  const sameSideNeedsParens =
    operand.precedence === parentPrecedence &&
    ((associativity === 'left' && side === 'right') || (associativity === 'right' && side === 'left'))
  if (!looser && !sameSideNeedsParens) return { text: operand.text, spans: [...operand.spans] }
  return { text: `(${operand.text})`, spans: shiftSpans(operand.spans, 1) }
}

function wrapParens(inner: EmittedExpression): EmittedExpression {
  return { text: `(${inner.text})`, precedence: PREC_ATOM, spans: shiftSpans(inner.spans, 1) }
}

function rejectFunctionOperand(ctx: GenerationContext, node: ts.Expression): void {
  const expression = unwrapTransparent(node)
  if (ts.isArrowFunction(expression) || ts.isFunctionExpression(expression)) {
    ctx.report(node, 'Functions inside operators are not supported.')
  }
}

function emitPrefix(ctx: GenerationContext, node: ts.PrefixUnaryExpression): EmittedExpression {
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
  const paren = parenthesize(operand, PREC_UNARY, 'right', 'right')
  const text = `${berry}${paren.text}`
  return withNode(node, { text, precedence: PREC_UNARY, spans: shiftSpans(paren.spans, berry.length) })
}

function emitBinary(ctx: GenerationContext, node: ts.BinaryExpression): EmittedExpression {
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
  const left = parenthesize(lowerExpression(ctx, node.left), operator.precedence, 'left', operator.associativity)
  const right = parenthesize(lowerExpression(ctx, node.right), operator.precedence, 'right', operator.associativity)
  const middle = ` ${operator.berry} `
  const text = `${left.text}${middle}${right.text}`
  return withNode(node, {
    text,
    precedence: operator.precedence,
    spans: [...left.spans, ...shiftSpans(right.spans, left.text + middle)],
  })
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

function emitArgument(ctx: GenerationContext, node: ts.Expression): EmittedExpression {
  if (ts.isSpreadElement(node)) {
    ctx.report(node, 'Spread syntax is not supported.')
    return atom('nil')
  }
  return lowerExpression(ctx, node)
}

function emitCall(ctx: GenerationContext, node: ts.CallExpression): EmittedExpression {
  if (node.questionDotToken) ctx.report(node, 'Optional chaining is not supported.')
  const callee = parenthesize(lowerExpression(ctx, node.expression), CALL_PRECEDENCE, 'left', 'left')
  const { text: argsText, spans: argSpans } = emitArguments(ctx, node.arguments)
  const text = `${callee.text}(${argsText})`
  return withNode(node, {
    text,
    precedence: CALL_PRECEDENCE,
    spans: [...callee.spans, ...shiftSpans(argSpans, callee.text.length + 1)],
  })
}

function emitNew(ctx: GenerationContext, node: ts.NewExpression): EmittedExpression {
  const callee = parenthesize(lowerExpression(ctx, node.expression), CALL_PRECEDENCE, 'left', 'left')
  const { text: argsText, spans: argSpans } = emitArguments(ctx, node.arguments ?? [])
  const text = `${callee.text}(${argsText})`
  return withNode(node, {
    text,
    precedence: CALL_PRECEDENCE,
    spans: [...callee.spans, ...shiftSpans(argSpans, callee.text.length + 1)],
  })
}

function emitArguments(ctx: GenerationContext, args: readonly ts.Expression[]): { text: string; spans: ExpressionSpan[] } {
  let text = ''
  const spans: ExpressionSpan[] = []
  for (let i = 0; i < args.length; i++) {
    const node = args[i]
    if (!node) continue
    if (i > 0) text += ', '
    const arg = emitArgument(ctx, node)
    spans.push(...shiftSpans(arg.spans, text))
    text += arg.text
  }
  return { text, spans }
}

function emitProperty(ctx: GenerationContext, node: ts.PropertyAccessExpression): EmittedExpression {
  if (node.questionDotToken) ctx.report(node, 'Optional chaining is not supported.')
  if (ts.isPrivateIdentifier(node.name)) {
    ctx.report(node.name, 'Private identifiers are not supported.')
    return atom('nil')
  }
  const object = parenthesize(lowerExpression(ctx, node.expression), CALL_PRECEDENCE, 'left', 'left')
  const called = isCallee(node)
  if (node.name.text === 'length' && !called) {
    // Berry global size() handles strings (byte length) and objects exposing size().
    // `obj.size()` would fail for plain Berry strings, so prefer `size(obj)`.
    const text = `size(${object.text})`
    return withNode(node, {
      text,
      precedence: CALL_PRECEDENCE,
      spans: shiftSpans(object.spans, 'size('),
    })
  }
  const text = `${object.text}.${node.name.text}`
  return withNode(node, { text, precedence: CALL_PRECEDENCE, spans: object.spans })
}

function emitElement(ctx: GenerationContext, node: ts.ElementAccessExpression): EmittedExpression {
  if (node.questionDotToken) ctx.report(node, 'Optional chaining is not supported.')
  const object = parenthesize(lowerExpression(ctx, node.expression), CALL_PRECEDENCE, 'left', 'left')
  const index = lowerExpression(ctx, node.argumentExpression)
  const text = `${object.text}[${index.text}]`
  return withNode(node, {
    text,
    precedence: CALL_PRECEDENCE,
    spans: [...object.spans, ...shiftSpans(index.spans, `${object.text}[`)],
  })
}

function isCallee(node: ts.Node): boolean {
  const { parent } = node
  return !!parent && ts.isCallExpression(parent) && parent.expression === node
}

function emitArray(ctx: GenerationContext, node: ts.ArrayLiteralExpression): EmittedExpression {
  let text = '['
  const spans: ExpressionSpan[] = []
  let first = true
  for (const element of node.elements) {
    if (ts.isOmittedExpression(element)) {
      ctx.report(element, 'Array holes are not supported.')
      continue
    }
    if (!first) text += ', '
    first = false
    if (ts.isSpreadElement(element)) {
      ctx.report(element, 'Spread syntax is not supported.')
      spans.push(...shiftSpans([{ line: 0, column: 0, node: element }], text))
      text += 'nil'
      continue
    }
    const emitted = lowerExpression(ctx, element)
    spans.push(...shiftSpans(emitted.spans, text))
    text += emitted.text
  }
  text += ']'
  return withNode(node, { text, precedence: PREC_ATOM, spans })
}

function emitObject(ctx: GenerationContext, node: ts.ObjectLiteralExpression): EmittedExpression {
  let text = '{'
  const spans: ExpressionSpan[] = []
  let first = true
  for (const property of node.properties) {
    if (!ts.isPropertyAssignment(property)) {
      ctx.report(property, 'Only explicit object fields are supported.')
      continue
    }
    const key = objectKey(ctx, property.name)
    if (!key) continue
    if (!first) text += ', '
    first = false
    text += `${key}: `
    const value = lowerExpression(ctx, property.initializer)
    spans.push(...shiftSpans(value.spans, text))
    text += value.text
  }
  text += '}'
  return withNode(node, { text, precedence: PREC_ATOM, spans })
}

function objectKey(ctx: GenerationContext, name: ts.PropertyName): string | undefined {
  if (ts.isIdentifier(name) || ts.isStringLiteral(name) || ts.isNumericLiteral(name)) {
    return berryString(name.text)
  }
  ctx.report(name, 'Computed property names are not supported.')
  return undefined
}

function emitTemplate(ctx: GenerationContext, node: ts.TemplateExpression): EmittedExpression {
  // Build as additive chain with span-aware parts.
  const parts: EmittedExpression[] = []
  if (node.head.text.length > 0) parts.push(atom(berryString(node.head.text), node.head))
  for (const span of node.templateSpans) {
    const value = lowerExpression(ctx, span.expression)
    parts.push(withNode(span.expression, { text: `str(${value.text})`, precedence: CALL_PRECEDENCE, spans: shiftSpans(value.spans, 'str(') }))
    if (span.literal.text.length > 0) parts.push(atom(berryString(span.literal.text), span.literal))
  }
  if (parts.length === 0) return atom('""', node)
  const [first, ...rest] = parts
  if (!first) return atom('""', node)
  let current = first
  for (const part of rest) {
    const left = parenthesize(current, PREC_ADDITIVE, 'left', 'left')
    const right = parenthesize(part, PREC_ADDITIVE, 'right', 'left')
    const middle = ' + '
    current = {
      text: `${left.text}${middle}${right.text}`,
      precedence: PREC_ADDITIVE,
      spans: [...left.spans, ...shiftSpans(right.spans, left.text + middle)],
    }
  }
  return withNode(node, current)
}
