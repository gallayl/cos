import ts from '@typescript/typescript6'

export function hasModifier(node: ts.Node, kind: ts.SyntaxKind): boolean {
  if (!ts.canHaveModifiers(node)) return false
  return (ts.getModifiers(node) ?? []).some((modifier) => modifier.kind === kind)
}

export function isDefaultExport(node: ts.Node): boolean {
  return hasModifier(node, ts.SyntaxKind.ExportKeyword) && hasModifier(node, ts.SyntaxKind.DefaultKeyword)
}

export function isNamedExport(node: ts.Node): boolean {
  return hasModifier(node, ts.SyntaxKind.ExportKeyword) && !hasModifier(node, ts.SyntaxKind.DefaultKeyword)
}

export function isAmbient(node: ts.Node): boolean {
  return hasModifier(node, ts.SyntaxKind.DeclareKeyword)
}

export function hasDecorators(node: ts.Node): boolean {
  return ts.canHaveDecorators(node) && (ts.getDecorators(node)?.length ?? 0) > 0
}

/** `++` and `--`, which are prefix or postfix unary expressions in the TypeScript AST. */
export function isIncrement(node: ts.Node): node is ts.PrefixUnaryExpression | ts.PostfixUnaryExpression {
  if (!ts.isPrefixUnaryExpression(node) && !ts.isPostfixUnaryExpression(node)) return false
  return node.operator === ts.SyntaxKind.PlusPlusToken || node.operator === ts.SyntaxKind.MinusMinusToken
}

/** let and const. `var` is function-scoped in both languages. */
export function isLexicalBinding(list: ts.VariableDeclarationList): boolean {
  return (list.flags & ts.NodeFlags.BlockScoped) !== 0
}
