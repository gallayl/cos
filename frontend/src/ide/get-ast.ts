import ts from "@typescript/typescript6";

export function getAst(scriptText: string) {
    const ast = ts.createSourceFile(
      "cos-script.ts",
      scriptText,
      ts.ScriptTarget.Latest,
      true,
      ts.ScriptKind.TS,
    );
  
    return ast;
  }
  