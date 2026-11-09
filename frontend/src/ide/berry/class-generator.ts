import ts from '@typescript/typescript6'

import type { GenerationContext } from './context'
import { beginFunction, endFunction, hasUnsupportedFunctionModifiers } from './function-emitter'
import { hasDecorators, hasModifier, isAmbient, isDefaultExport, isNamedExport } from './syntax'

export interface CosClassInfo {
  name: string
  isDefaultExport: boolean
  hasSetup: boolean
  hasRun: boolean
  node: ts.ClassDeclaration
}

/**
 * @brief Lower a class to a Berry class. Instance field initializers run in `init`.
 * TS-only `extends CosApplication/CosUiApplication` is accepted and not emitted.
 */
export function emitClass(ctx: GenerationContext, node: ts.ClassDeclaration): CosClassInfo | undefined {
  if (isAmbient(node)) return undefined
  if (hasDecorators(node)) ctx.report(node, 'Decorators are not supported.', 'error', 'cos/no-decorators')
  if (hasModifier(node, ts.SyntaxKind.AbstractKeyword)) {
    ctx.report(node, 'Abstract classes are not supported.', 'error', 'cos/no-abstract-class')
  }
  for (const clause of node.heritageClauses ?? []) {
    if (clause.token === ts.SyntaxKind.ExtendsKeyword) {
      if (!isAllowedTsOnlyExtends(clause)) {
        ctx.report(clause, 'Class inheritance is not supported in Berry output.', 'error', 'cos/no-extends')
      }
      continue
    }
    // implements is TS-only, ignored.
  }
  const name = node.name?.text ?? (isDefaultExport(node) ? ctx.nextTemp('__cos_default_') : undefined)
  if (!name) {
    ctx.report(node, 'Classes must be named.', 'error', 'cos/class-name')
    return undefined
  }
  if (node.name) ctx.declareName(node.name.text, node.name)
  else ctx.declareName(name, node)
  const isDefault = isDefaultExport(node)
  if (isDefault) ctx.noteDefaultExport(name, node)
  else if (isNamedExport(node) && node.name) ctx.noteNamedExport(node.name.text, node.name.text, node)

  const fields: Array<{ name: string; initializer?: ts.Expression }> = []
  const methods: ts.MethodDeclaration[] = []
  let constructor: ts.ConstructorDeclaration | undefined
  let sawConstructor = false
  let hasSetup = false
  let hasRun = false

  for (const member of node.members) {
    if (ts.isSemicolonClassElement(member) || ts.isIndexSignatureDeclaration(member)) continue
    if (isAmbient(member)) continue
    if (hasModifier(member, ts.SyntaxKind.StaticKeyword)) {
      const staticName = memberName(member)
      if (staticName === 'setup') hasSetup = hasSetup // static setup not an entrypoint
      // Static members (metadata) are allowed as TS-only declarations if not executable in Berry?
      // Berry has no static; skip emitting but don't fail for simple metadata fields.
      if (ts.isPropertyDeclaration(member) && ts.isIdentifier(member.name) && !member.initializer) continue
      if (ts.isPropertyDeclaration(member) && ts.isIdentifier(member.name) && member.initializer) {
        ctx.report(
          member,
          'Static members with runtime initializers are not supported yet.',
          'error',
          'cos/no-static-runtime',
        )
      } else if (ts.isMethodDeclaration(member)) {
        ctx.report(member, 'Static methods are not supported yet.', 'error', 'cos/no-static-method')
      } else {
        ctx.report(member, 'Static members are not supported yet.', 'error', 'cos/no-static')
      }
      continue
    }
    if (hasDecorators(member)) ctx.report(member, 'Decorators are not supported.', 'error', 'cos/no-decorators')

    if (ts.isConstructorDeclaration(member)) {
      if (member.body) {
        if (sawConstructor) ctx.report(member, 'Only one constructor is supported.', 'error', 'cos/one-constructor')
        else constructor = member
        sawConstructor = true
      }
      continue
    }
    if (ts.isMethodDeclaration(member)) {
      const methodName = memberName(member)
      if (methodName === 'setup') hasSetup = true
      if (methodName === 'run') hasRun = true
      methods.push(member)
      continue
    }
    if (ts.isPropertyDeclaration(member)) {
      const field = propertyField(ctx, member)
      if (field) fields.push(field)
      continue
    }
    if (ts.isGetAccessorDeclaration(member) || ts.isSetAccessorDeclaration(member)) {
      ctx.report(member, 'Accessors are not supported.', 'error', 'cos/no-accessors')
      continue
    }
    if (ts.isClassStaticBlockDeclaration(member)) {
      ctx.report(member, 'Static blocks are not supported.', 'error', 'cos/no-static-block')
      continue
    }
    ctx.report(member, `Unsupported syntax: ${ts.SyntaxKind[member.kind]}.`, 'error', 'cos/unsupported')
  }

  ctx.writeLine(`class ${name}`)
  ctx.indent()
  for (const field of fields) ctx.writeLine(`var ${field.name}`)

  const initializers = fields.filter((field) => field.initializer !== undefined)
  if (constructor || initializers.length > 0) {
    const parameters = constructor?.parameters ?? []
    if (beginFunction(ctx, 'def init', parameters)) {
      for (const field of initializers) {
        if (!field.initializer) continue
        ctx.writeLine(`self.${field.name} = ${ctx.emitExpression(field.initializer)}`)
      }
      for (const statement of constructor?.body?.statements ?? []) {
        ctx.emitStatement(statement)
      }
      endFunction(ctx)
    }
  }

  for (const method of methods) emitMethod(ctx, method)
  ctx.dedent()
  ctx.writeLine('end')

  return { name, isDefaultExport: isDefault, hasSetup, hasRun, node }
}

function isAllowedTsOnlyExtends(clause: ts.HeritageClause): boolean {
  const type = clause.types[0]?.expression
  if (!type) return false
  if (ts.isIdentifier(type)) {
    return type.text === 'CosApplication' || type.text === 'CosUiApplication'
  }
  return false
}

function memberName(member: ts.ClassElement): string | undefined {
  if ((ts.isMethodDeclaration(member) || ts.isPropertyDeclaration(member)) && ts.isIdentifier(member.name)) {
    return member.name.text
  }
  return undefined
}

function propertyField(
  ctx: GenerationContext,
  node: ts.PropertyDeclaration,
): { name: string; initializer?: ts.Expression } | undefined {
  if (!ts.isIdentifier(node.name)) {
    ctx.report(node.name, 'Only identifier property names are supported.', 'error', 'cos/property-name')
    return undefined
  }
  return node.initializer ? { name: node.name.text, initializer: node.initializer } : { name: node.name.text }
}

function emitMethod(ctx: GenerationContext, node: ts.MethodDeclaration): void {
  if (!node.body || hasUnsupportedFunctionModifiers(ctx, node)) return
  if (!ts.isIdentifier(node.name)) {
    ctx.report(node.name, 'Only identifier method names are supported.', 'error', 'cos/method-name')
    return
  }
  const header = `def ${node.name.text}`
  if (!beginFunction(ctx, header, node.parameters)) return
  for (const statement of node.body.statements) ctx.emitStatement(statement)
  endFunction(ctx)
}
