import ts from "@typescript/typescript6";

import type { GenerationContext } from "./context";
import { beginFunction, endFunction, hasUnsupportedFunctionModifiers } from "./function-emitter";
import { hasDecorators, hasModifier, isAmbient, isDefaultExport } from "./syntax";

/**
 * @brief Lower a class to a Berry class. Instance field initializers run in `init`.
 */
export function emitClass(ctx: GenerationContext, node: ts.ClassDeclaration): void {
  if (isAmbient(node)) return;
  if (hasDecorators(node)) ctx.report(node, "Decorators are not supported.");
  if (hasModifier(node, ts.SyntaxKind.AbstractKeyword)) {
    ctx.report(node, "Abstract classes are not supported.");
  }
  for (const clause of node.heritageClauses ?? []) {
    if (clause.token === ts.SyntaxKind.ExtendsKeyword) {
      ctx.report(clause, "Single inheritance is not supported yet.");
    }
  }
  if (!node.name) {
    ctx.report(node, "Classes must be named.");
    return;
  }
  if (isDefaultExport(node)) ctx.noteDefaultExport(node.name.text, node);
  ctx.declareName(node.name.text, node.name);

  const fields: Array<{ name: string; initializer?: ts.Expression }> = [];
  const methods: ts.MethodDeclaration[] = [];
  let constructor: ts.ConstructorDeclaration | undefined;
  let sawConstructor = false;

  for (const member of node.members) {
    if (ts.isSemicolonClassElement(member) || ts.isIndexSignatureDeclaration(member)) continue;
    if (isAmbient(member)) continue;
    if (hasModifier(member, ts.SyntaxKind.StaticKeyword)) {
      ctx.report(member, "Static members are not supported yet.");
      continue;
    }
    if (hasDecorators(member)) ctx.report(member, "Decorators are not supported.");

    if (ts.isConstructorDeclaration(member)) {
      if (member.body) {
        if (sawConstructor) ctx.report(member, "Only one constructor is supported.");
        else constructor = member;
        sawConstructor = true;
      }
      continue;
    }
    if (ts.isMethodDeclaration(member)) {
      methods.push(member);
      continue;
    }
    if (ts.isPropertyDeclaration(member)) {
      const field = propertyField(ctx, member);
      if (field) fields.push(field);
      continue;
    }
    if (ts.isGetAccessorDeclaration(member) || ts.isSetAccessorDeclaration(member)) {
      ctx.report(member, "Accessors are not supported.");
      continue;
    }
    if (ts.isClassStaticBlockDeclaration(member)) {
      ctx.report(member, "Static blocks are not supported.");
      continue;
    }
    ctx.report(member, `Unsupported syntax: ${ts.SyntaxKind[member.kind]}.`);
  }

  ctx.writeLine(`class ${node.name.text}`);
  ctx.indent();
  for (const field of fields) ctx.writeLine(`var ${field.name}`);

  const initializers = fields.filter((field) => field.initializer !== undefined);
  if (constructor || initializers.length > 0) {
    const parameters = constructor?.parameters ?? [];
    if (beginFunction(ctx, "def init", parameters)) {
      for (const field of initializers) {
        if (!field.initializer) continue;
        ctx.writeLine(`self.${field.name} = ${ctx.emitExpression(field.initializer)}`);
      }
      for (const statement of constructor?.body?.statements ?? []) {
        ctx.emitStatement(statement);
      }
      endFunction(ctx);
    }
  }

  for (const method of methods) emitMethod(ctx, method);
  ctx.dedent();
  ctx.writeLine("end");
}

function propertyField(
  ctx: GenerationContext,
  node: ts.PropertyDeclaration,
): { name: string; initializer?: ts.Expression } | undefined {
  if (!ts.isIdentifier(node.name)) {
    ctx.report(node.name, "Only identifier property names are supported.");
    return undefined;
  }
  return node.initializer
    ? { name: node.name.text, initializer: node.initializer }
    : { name: node.name.text };
}

function emitMethod(ctx: GenerationContext, node: ts.MethodDeclaration): void {
  if (!node.body || hasUnsupportedFunctionModifiers(ctx, node)) return;
  if (!ts.isIdentifier(node.name)) {
    ctx.report(node.name, "Only identifier method names are supported.");
    return;
  }
  const header = `def ${node.name.text}`;
  if (!beginFunction(ctx, header, node.parameters)) return;
  for (const statement of node.body.statements) ctx.emitStatement(statement);
  endFunction(ctx);
}
