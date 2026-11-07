import ts from "@typescript/typescript6";

import { BOOLEAN_CONDITION_MESSAGE, isStructurallyBoolean } from "./boolean-expression";
import { emitClass } from "./class-generator";
import type { GenerationContext } from "./context";
import { CALL_PRECEDENCE, emitGrouped } from "./expression-generator";
import { beginFunction, endFunction, hasUnsupportedFunctionModifiers } from "./function-emitter";
import { hasDecorators, isAmbient, isDefaultExport, isIncrement, isLexicalBinding } from "./syntax";

const LOOP_CLOSURE_MESSAGE =
  "Closures that capture for-loop bindings are not supported.";

/**
 * @brief Lower one TypeScript statement to Berry statements.
 */
export function emitStatement(ctx: GenerationContext, node: ts.Statement): void {
  if (isAmbient(node)) return;
  if (ts.isEmptyStatement(node)) return;

  if (ts.isBlock(node)) {
    ctx.pushScope();
    for (const statement of node.statements) ctx.emitStatement(statement);
    ctx.popScope();
    return;
  }
  if (ts.isVariableStatement(node)) {
    emitVariableDeclarationList(ctx, node.declarationList);
    return;
  }
  if (ts.isFunctionDeclaration(node)) {
    emitFunctionDeclaration(ctx, node);
    return;
  }
  if (ts.isClassDeclaration(node)) {
    emitClassDeclaration(ctx, node);
    return;
  }
  if (ts.isIfStatement(node)) {
    emitIf(ctx, node, "if");
    return;
  }
  if (ts.isWhileStatement(node)) {
    emitWhile(ctx, node);
    return;
  }
  if (ts.isForStatement(node)) {
    emitFor(ctx, node);
    return;
  }
  if (ts.isForOfStatement(node)) {
    emitForOf(ctx, node);
    return;
  }
  if (ts.isForInStatement(node)) {
    ctx.report(node, "for-in is not supported. Use for-of.");
    return;
  }
  if (ts.isSwitchStatement(node)) {
    emitSwitch(ctx, node);
    return;
  }
  if (ts.isExpressionStatement(node)) {
    if (isIncrement(node.expression)) writeUpdate(ctx, node.expression);
    else ctx.writeLine(ctx.emitExpression(node.expression));
    return;
  }
  if (ts.isReturnStatement(node)) {
    ctx.writeLine(
      node.expression ? `return ${ctx.emitExpression(node.expression)}` : "return",
    );
    return;
  }
  if (ts.isBreakStatement(node) || ts.isContinueStatement(node)) {
    if (node.label) {
      ctx.report(node, "Labels are not supported.");
      return;
    }
    ctx.writeLine(ts.isBreakStatement(node) ? "break" : "continue");
    return;
  }
  if (ts.isEnumDeclaration(node)) {
    ctx.report(node, "Enums are not supported yet.");
    return;
  }
  if (ts.isInterfaceDeclaration(node) || ts.isTypeAliasDeclaration(node)) return;
  if (ts.isImportDeclaration(node) || ts.isImportEqualsDeclaration(node)) {
    ctx.report(node, "Imports are not supported. COS APIs are available as globals.");
    return;
  }
  if (ts.isExportDeclaration(node)) {
    ctx.report(node, "Named exports are not supported.");
    return;
  }
  if (ts.isExportAssignment(node)) {
    emitExportAssignment(ctx, node);
    return;
  }
  if (ts.isModuleDeclaration(node)) {
    ctx.report(node, "Namespaces are not supported.");
    return;
  }
  if (ts.isThrowStatement(node)) {
    ctx.report(node, "Throw is not supported.");
    return;
  }
  if (ts.isTryStatement(node)) {
    ctx.report(node, "Try statements are not supported.");
    return;
  }
  if (ts.isDoStatement(node)) {
    ctx.report(node, "Do-while loops are not supported.");
    return;
  }
  if (node.kind === ts.SyntaxKind.DebuggerStatement) {
    ctx.report(node, "Debugger statements are not supported.");
    return;
  }

  ctx.report(node, `Unsupported syntax: ${ts.SyntaxKind[node.kind]}.`);
}

function emitFunctionDeclaration(ctx: GenerationContext, node: ts.FunctionDeclaration): void {
  if (!node.name) {
    ctx.report(node, "Functions must be named.");
    return;
  }
  if (isDefaultExport(node)) {
    ctx.report(node, "Only a default class export is supported.");
  }
  if (hasDecorators(node)) ctx.report(node, "Decorators are not supported.");
  if (hasUnsupportedFunctionModifiers(ctx, node) || !node.body) return;
  ctx.declareName(node.name.text, node.name);
  if (!beginFunction(ctx, `def ${node.name.text}`, node.parameters)) return;
  for (const statement of node.body.statements) ctx.emitStatement(statement);
  endFunction(ctx);
}

function emitExportAssignment(ctx: GenerationContext, node: ts.ExportAssignment): void {
  if (node.isExportEquals) {
    ctx.report(node, "export = is not supported.");
    return;
  }
  if (ts.isIdentifier(node.expression)) {
    ctx.noteDefaultExport(node.expression.text, node);
    return;
  }
  ctx.report(node, "Only a default class export is supported.");
}

function emitVariableDeclarationList(
  ctx: GenerationContext,
  list: ts.VariableDeclarationList,
): void {
  if (list.flags & ts.NodeFlags.Using) {
    ctx.report(list, "using declarations are not supported.");
    return;
  }
  for (const declaration of list.declarations) {
    if (!ts.isIdentifier(declaration.name)) {
      ctx.report(declaration, "Destructuring is not supported.");
      continue;
    }
    ctx.declareName(declaration.name.text, declaration.name);
    if (declaration.initializer) {
      ctx.writeLine(
        `var ${declaration.name.text} = ${ctx.emitExpression(declaration.initializer)}`,
      );
    } else {
      ctx.writeLine(`var ${declaration.name.text}`);
    }
  }
}

function emitIf(
  ctx: GenerationContext,
  node: ts.IfStatement,
  keyword: "if" | "elif",
): void {
  if (!isStructurallyBoolean(node.expression)) {
    ctx.report(node.expression, BOOLEAN_CONDITION_MESSAGE);
  }
  ctx.writeLine(`${keyword} ${ctx.emitExpression(node.expression)}`);
  ctx.indent();
  ctx.emitStatement(node.thenStatement);
  ctx.dedent();
  if (node.elseStatement && ts.isIfStatement(node.elseStatement)) {
    emitIf(ctx, node.elseStatement, "elif");
  } else if (node.elseStatement) {
    ctx.writeLine("else");
    ctx.indent();
    ctx.emitStatement(node.elseStatement);
    ctx.dedent();
  }
  if (keyword === "if") ctx.writeLine("end");
}

function emitWhile(ctx: GenerationContext, node: ts.WhileStatement): void {
  if (!isStructurallyBoolean(node.expression)) {
    ctx.report(node.expression, BOOLEAN_CONDITION_MESSAGE);
  }
  ctx.writeLine(`while ${ctx.emitExpression(node.expression)}`);
  ctx.indent();
  ctx.emitStatement(node.statement);
  ctx.dedent();
  ctx.writeLine("end");
}

function emitFor(ctx: GenerationContext, node: ts.ForStatement): void {
  if (capturesLoopBinding(node)) ctx.report(node, LOOP_CLOSURE_MESSAGE);
  ctx.pushScope();
  if (node.initializer) {
    if (ts.isVariableDeclarationList(node.initializer)) {
      emitVariableDeclarationList(ctx, node.initializer);
    } else {
      ctx.writeLine(ctx.emitExpression(node.initializer));
    }
  }
  if (node.condition && !isStructurallyBoolean(node.condition)) {
    ctx.report(node.condition, BOOLEAN_CONDITION_MESSAGE);
  }
  const condition = node.condition ? ctx.emitExpression(node.condition) : "true";
  ctx.writeLine(`while ${condition}`);
  ctx.indent();
  ctx.emitStatement(node.statement);
  if (node.incrementor) writeStep(ctx, node.incrementor);
  ctx.dedent();
  ctx.writeLine("end");
  ctx.popScope();
}

function emitForOf(ctx: GenerationContext, node: ts.ForOfStatement): void {
  if (node.awaitModifier) ctx.report(node, "for-await is not supported.");
  const binding = forOfBinding(ctx, node.initializer);
  if (!binding) return;
  if (capturesLoopBinding(node)) ctx.report(node, LOOP_CLOSURE_MESSAGE);
  const target = emitGrouped(ctx, node.expression, CALL_PRECEDENCE);
  ctx.pushScope();
  ctx.declareName(binding.name, binding.node);
  ctx.writeLine(`for ${binding.name}: ${target}`);
  ctx.indent();
  ctx.emitStatement(node.statement);
  ctx.dedent();
  ctx.writeLine("end");
  ctx.popScope();
}

function forOfBinding(
  ctx: GenerationContext,
  initializer: ts.ForInitializer,
): { name: string; node: ts.Node } | undefined {
  if (ts.isVariableDeclarationList(initializer)) {
    if (initializer.declarations.length !== 1) {
      ctx.report(initializer, "for-of must declare a single variable.");
      return undefined;
    }
    const declaration = initializer.declarations[0];
    if (!declaration || !ts.isIdentifier(declaration.name)) {
      ctx.report(initializer, "Destructuring is not supported.");
      return undefined;
    }
    return { name: declaration.name.text, node: declaration.name };
  }
  if (ts.isIdentifier(initializer)) return { name: initializer.text, node: initializer };
  ctx.report(initializer, "Destructuring is not supported.");
  return undefined;
}

function capturesLoopBinding(node: ts.ForStatement | ts.ForOfStatement): boolean {
  if (
    !node.initializer ||
    !ts.isVariableDeclarationList(node.initializer) ||
    !isLexicalBinding(node.initializer)
  ) {
    return false;
  }
  if (containsNewClosure(node.statement)) return true;
  return ts.isForStatement(node) && !!node.incrementor && containsNewClosure(node.incrementor);
}

function containsNewClosure(node: ts.Node): boolean {
  if (ts.isFunctionLike(node)) return true;
  let found = false;
  const visit = (current: ts.Node): void => {
    if (found) return;
    if (ts.isFunctionLike(current)) {
      found = true;
      return;
    }
    ts.forEachChild(current, visit);
  };
  ts.forEachChild(node, visit);
  return found;
}

function writeStep(ctx: GenerationContext, expression: ts.Expression): void {
  if (isIncrement(expression)) writeUpdate(ctx, expression);
  else ctx.writeLine(ctx.emitExpression(expression));
}

function writeUpdate(
  ctx: GenerationContext,
  node: ts.PrefixUnaryExpression | ts.PostfixUnaryExpression,
): void {
  const operator = node.operator === ts.SyntaxKind.PlusPlusToken ? "+=" : "-=";
  ctx.writeLine(`${ctx.emitExpression(node.operand)} ${operator} 1`);
}

interface SwitchArm {
  tests: readonly ts.Expression[];
  isDefault: boolean;
  statements: readonly ts.Statement[];
}

function emitSwitch(ctx: GenerationContext, node: ts.SwitchStatement): void {
  const arms = collectSwitchArms(ctx, node);
  if (!arms) return;

  const name = ctx.nextTemp("__cos_switch_");
  ctx.declareName(name, node);
  ctx.writeLine(`var ${name} = ${ctx.emitExpression(node.expression)}`);

  const conditional = arms.filter((arm) => !arm.isDefault);
  const defaultArm = arms.find((arm) => arm.isDefault);
  ctx.pushScope();
  if (conditional.length === 0) {
    for (const statement of defaultArm?.statements ?? []) ctx.emitStatement(statement);
    ctx.popScope();
    return;
  }

  conditional.forEach((arm, index) => {
    const keyword = index === 0 ? "if" : "elif";
    const test = arm.tests
      .map((expression) => `(${name} == ${emitGrouped(ctx, expression, CALL_PRECEDENCE)})`)
      .join(" || ");
    ctx.writeLine(`${keyword} ${test}`);
    ctx.indent();
    for (const statement of arm.statements) ctx.emitStatement(statement);
    ctx.dedent();
  });
  if (defaultArm) {
    ctx.writeLine("else");
    ctx.indent();
    for (const statement of defaultArm.statements) ctx.emitStatement(statement);
    ctx.dedent();
  }
  ctx.writeLine("end");
  ctx.popScope();
}

function collectSwitchArms(
  ctx: GenerationContext,
  node: ts.SwitchStatement,
): readonly SwitchArm[] | undefined {
  const arms: SwitchArm[] = [];
  let tests: ts.Expression[] = [];
  let isDefault = false;
  const clauses = node.caseBlock.clauses;

  for (let index = 0; index < clauses.length; index += 1) {
    const clause = clauses[index];
    if (!clause) continue;
    if (ts.isDefaultClause(clause)) isDefault = true;
    else tests.push(clause.expression);

    const split = splitCaseStatements(clause.statements);
    if (split.illegal) {
      ctx.report(clause, "Switch fallthrough is not supported.");
      return undefined;
    }
    const isLast = index === clauses.length - 1;
    const terminated =
      split.explicitBreak || terminatesClause(split.statements[split.statements.length - 1]);
    if (split.statements.length === 0 && !terminated && !isLast) continue;
    if (split.statements.length > 0 && !terminated && !isLast) {
      ctx.report(clause, "Switch fallthrough is not supported.");
      return undefined;
    }
    arms.push({ tests: tests.slice(), isDefault, statements: split.statements });
    tests = [];
    isDefault = false;
  }
  return arms;
}

function splitCaseStatements(statements: readonly ts.Statement[]): {
  statements: ts.Statement[];
  explicitBreak: boolean;
  illegal: boolean;
} {
  const body: ts.Statement[] = [];
  for (let index = 0; index < statements.length; index += 1) {
    const statement = statements[index];
    if (!statement || statement.kind === ts.SyntaxKind.EmptyStatement) continue;
    if (!ts.isBreakStatement(statement)) {
      body.push(statement);
      continue;
    }
    if (statement.label) return { statements: body, explicitBreak: true, illegal: true };
    const trailing = statements
      .slice(index + 1)
      .some((item) => item.kind !== ts.SyntaxKind.EmptyStatement);
    return { statements: body, explicitBreak: true, illegal: trailing };
  }
  return { statements: body, explicitBreak: false, illegal: false };
}

function terminatesClause(statement: ts.Statement | undefined): boolean {
  if (!statement) return false;
  if (
    ts.isReturnStatement(statement) ||
    ts.isThrowStatement(statement) ||
    ts.isBreakStatement(statement) ||
    ts.isContinueStatement(statement)
  ) {
    return true;
  }
  if (!ts.isBlock(statement)) return false;
  const last = [...statement.statements]
    .reverse()
    .find((item) => item.kind !== ts.SyntaxKind.EmptyStatement);
  return terminatesClause(last);
}

function emitClassDeclaration(ctx: GenerationContext, node: ts.ClassDeclaration): void {
  emitClass(ctx, node);
}
