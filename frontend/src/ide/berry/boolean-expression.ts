import ts from '@typescript/typescript6'

/** Berry treats only `false` and `nil` as falsy, so truthy numbers and strings are rejected. */
export const BOOLEAN_CONDITION_MESSAGE =
  'Conditions must be boolean. Compare values explicitly; Berry treats only false and nil as falsy.'

export const BOOLEAN_OPERAND_MESSAGE =
  'This operator needs a boolean operand. Compare values explicitly; Berry treats only false and nil as falsy.'

/** Strip groupings and type syntax that have no runtime value. */
export function unwrapTransparent(node: ts.Expression): ts.Expression {
  let current = node
  while (
    ts.isParenthesizedExpression(current) ||
    ts.isAsExpression(current) ||
    ts.isTypeAssertionExpression(current) ||
    ts.isSatisfiesExpression(current) ||
    ts.isNonNullExpression(current)
  ) {
    current = current.expression
  }
  return current
}

/**
 * @brief True when a condition is safe to lower.
 * Comparisons, booleans, and references are accepted. Numeric and string
 * literals are rejected because `0` and `""` are falsy in TypeScript and
 * truthy in Berry. `.length` is rejected because it lowers to numeric `size()`.
 * A later type checker can tighten references that are not actually boolean.
 */
export function isStructurallyBoolean(node: ts.Expression): boolean {
  const expression = unwrapTransparent(node)
  if (
    expression.kind === ts.SyntaxKind.TrueKeyword ||
    expression.kind === ts.SyntaxKind.FalseKeyword ||
    ts.isIdentifier(expression) ||
    ts.isCallExpression(expression) ||
    ts.isNewExpression(expression) ||
    ts.isElementAccessExpression(expression)
  ) {
    return true
  }
  if (ts.isPropertyAccessExpression(expression)) {
    return !ts.isIdentifier(expression.name) || expression.name.text !== 'length'
  }
  if (ts.isPrefixUnaryExpression(expression) && expression.operator === ts.SyntaxKind.ExclamationToken) {
    return isStructurallyBoolean(expression.operand)
  }
  if (!ts.isBinaryExpression(expression)) return false

  switch (expression.operatorToken.kind) {
    case ts.SyntaxKind.EqualsEqualsEqualsToken:
    case ts.SyntaxKind.ExclamationEqualsEqualsToken:
    case ts.SyntaxKind.LessThanToken:
    case ts.SyntaxKind.GreaterThanToken:
    case ts.SyntaxKind.LessThanEqualsToken:
    case ts.SyntaxKind.GreaterThanEqualsToken:
      return true
    case ts.SyntaxKind.AmpersandAmpersandToken:
    case ts.SyntaxKind.BarBarToken:
      return isStructurallyBoolean(expression.left) && isStructurallyBoolean(expression.right)
    default:
      return false
  }
}
