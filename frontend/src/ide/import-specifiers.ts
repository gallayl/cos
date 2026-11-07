import ts from "@typescript/typescript6";

/** Collect relative module specifiers from a TypeScript source. */
export function collectRelativeImportSpecifiers(source: string): string[] {
  const sourceFile = ts.createSourceFile(
    "imports.ts",
    source,
    ts.ScriptTarget.Latest,
    true,
    ts.ScriptKind.TS,
  );
  const specifiers = new Set<string>();

  const visit = (node: ts.Node): void => {
    if (
      (ts.isImportDeclaration(node) || ts.isExportDeclaration(node)) &&
      node.moduleSpecifier &&
      ts.isStringLiteral(node.moduleSpecifier)
    ) {
      const value = node.moduleSpecifier.text;
      if (value.startsWith("./") || value.startsWith("../")) {
        specifiers.add(value);
      }
    }
    if (ts.isImportEqualsDeclaration(node) && ts.isExternalModuleReference(node.moduleReference)) {
      const expression = node.moduleReference.expression;
      if (expression && ts.isStringLiteral(expression)) {
        const value = expression.text;
        if (value.startsWith("./") || value.startsWith("../")) {
          specifiers.add(value);
        }
      }
    }
    ts.forEachChild(node, visit);
  };

  visit(sourceFile);
  return [...specifiers];
}
