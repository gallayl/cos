import { execFileSync } from "node:child_process";
import { mkdirSync, mkdtempSync, readdirSync, readFileSync, rmSync, statSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, relative, sep } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

import { astToBerry } from "../src/ide/ast-to-berry";
import { getAst } from "../src/ide/get-ast";

const repoRoot = fileURLToPath(new URL("../..", import.meta.url));
const storageRoot = join(repoRoot, "storage");
const flashRoot = join(storageRoot, "flash_data");
const typescriptRoot = join(storageRoot, "ts");
const configPath = join(storageRoot, "tsconfig.json");
const tscPath = fileURLToPath(new URL("../node_modules/typescript/bin/tsc", import.meta.url));
const sdkIndex = fileURLToPath(new URL("../src/cos-sdk/index.d.ts", import.meta.url));

const OPERATORS = ["&&", "||", "==", "!=", "<=", ">=", "+=", "-=", "*=", "/=", "%="];

export interface ScriptCheck {
  /** Path relative to `storage/ts` and `storage/flash_data`, using `/`. */
  name: string;
  diagnostics: readonly string[];
  /** Empty when the compiled Berry matches the device script. */
  diff: string;
}

export interface StorageCheck {
  optionProblems: readonly string[];
  /** Diagnostics from `storage/tsconfig.json`, SDK included. */
  typecheck: readonly string[];
  /** Diagnostics for a snippet that uses JavaScript globals. */
  nonSdk: readonly string[];
  scripts: readonly ScriptCheck[];
}

/**
 * Typecheck every storage script against the COS SDK, compile each one to
 * Berry, and compare that output with the device script.
 */
export function verifyStorage(): StorageCheck {
  const config = JSON.parse(readFileSync(configPath, "utf8")) as {
    compilerOptions?: Record<string, unknown>;
  };
  const typecheck = runTsc(configPath);
  return {
    optionProblems: compilerOptionProblems(config.compilerOptions ?? {}),
    typecheck: typecheck.status === 0 ? [] : outputLines(typecheck.output),
    nonSdk: nonSdkDiagnostics(config.compilerOptions ?? {}),
    scripts: checkScripts(),
  };
}

function writeCompiledBerry(): number {
  const names = [...collect(typescriptRoot, ".ts").keys()].sort();
  let written = 0;
  for (const name of names) {
    const typescriptPath = join(typescriptRoot, `${name}.ts`);
    const berryPath = join(flashRoot, `${name}.be`);
    const result = astToBerry(getAst(readFileSync(typescriptPath, "utf8")));
    if (!result.ok) {
      console.error(
        `${name}: ${result.diagnostics.map((d) => `${d.line}:${d.column}: ${d.message}`).join("; ")}`,
      );
      return 1;
    }
    mkdirSync(join(berryPath, ".."), { recursive: true });
    writeFileSync(berryPath, result.code.endsWith("\n") ? result.code : `${result.code}\n`);
    written += 1;
  }
  console.log(`Wrote ${written} Berry files to storage/flash_data`);
  return 0;
}

/** @returns Process exit code: `0` when every check passed. */
export function main(args: readonly string[] = process.argv.slice(2)): number {
  if (args.includes("--write")) {
    return writeCompiledBerry();
  }
  const report = verifyStorage();
  const problems = reportProblems(report);
  if (problems.length === 0) {
    console.log(`COS storage scripts: ${report.scripts.length} TypeScript files compiled to Berry`);
    return 0;
  }
  console.error(problems.join("\n"));
  return 1;
}

function reportProblems(report: StorageCheck): string[] {
  const problems = [
    ...report.optionProblems,
    ...report.typecheck,
    ...report.scripts.flatMap((script) => [
      ...script.diagnostics.map((message) => `${script.name}: ${message}`),
      ...(script.diff ? [`${script.name}\n${script.diff}`] : []),
    ]),
  ];
  const nonSdk = report.nonSdk.join("\n");
  if (!nonSdk.includes("console") || !nonSdk.includes("setTimeout") || !nonSdk.includes("Math")) {
    problems.push(`COS SDK only check did not reject JavaScript globals:\n${nonSdk}`);
  }
  return problems;
}

function compilerOptionProblems(options: Record<string, unknown>): string[] {
  const problems: string[] = [];
  if (options.noLib !== true) problems.push("storage/tsconfig.json must set noLib");
  if (options.strict !== true) problems.push("storage/tsconfig.json must set strict");
  if (options.lib !== undefined) problems.push("storage/tsconfig.json must not set lib");
  if (!Array.isArray(options.types) || options.types.length > 0) {
    problems.push("storage/tsconfig.json must set types to an empty list");
  }
  if (options.moduleDetection !== "force") {
    problems.push("storage/tsconfig.json must force module detection");
  }
  return problems;
}

function nonSdkDiagnostics(options: Record<string, unknown>): string[] {
  const dir = mkdtempSync(join(tmpdir(), "cos-sdk-"));
  try {
    writeFileSync(join(dir, "guard.ts"), `console.log("nope");\nsetTimeout(() => {}, 1);\nMath.random();\n`);
    writeFileSync(
      join(dir, "tsconfig.json"),
      JSON.stringify(
        {
          compilerOptions: options,
          files: ["guard.ts", sdkIndex],
        },
        null,
        2,
      ),
    );
    return outputLines(runTsc(join(dir, "tsconfig.json")).output);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
}

function runTsc(project: string): { status: number; output: string } {
  try {
    const output = execFileSync(process.execPath, [tscPath, "-p", project, "--noEmit", "--pretty", "false"], {
      encoding: "utf8",
    });
    return { status: 0, output };
  } catch (error) {
    const failed = error as { status?: number; stdout?: string; stderr?: string };
    return { status: failed.status ?? 1, output: `${failed.stdout ?? ""}\n${failed.stderr ?? ""}` };
  }
}

function outputLines(output: string): string[] {
  return output
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line.length > 0);
}

function checkScripts(): ScriptCheck[] {
  const berry = collect(flashRoot, ".be");
  const typescript = collect(typescriptRoot, ".ts");
  const names = [...new Set([...berry.keys(), ...typescript.keys()])].sort();
  return names.map((name) => checkOne(name, berry.get(name), typescript.get(name)));
}

function checkOne(name: string, berryPath: string | undefined, typescriptPath: string | undefined): ScriptCheck {
  if (!berryPath || !typescriptPath) {
    const missing = berryPath ? "TypeScript source" : "Berry script";
    return { name, diagnostics: [`Missing ${missing}`], diff: "" };
  }
  const result = astToBerry(getAst(readFileSync(typescriptPath, "utf8")));
  if (!result.ok) {
    return {
      name,
      diagnostics: result.diagnostics.map(
        (diagnostic) => `${diagnostic.line}:${diagnostic.column}: ${diagnostic.message}`,
      ),
      diff: "",
    };
  }
  const expected = semanticTokens(readFileSync(berryPath, "utf8"));
  const actual = semanticTokens(result.code);
  return { name, diagnostics: [], diff: tokenDiff(expected, actual) };
}

function collect(root: string, suffix: string): Map<string, string> {
  const files = new Map<string, string>();
  for (const file of walk(root)) {
    if (!file.endsWith(suffix)) continue;
    const key = relative(root, file.slice(0, -suffix.length)).split(sep).join("/");
    files.set(key, file);
  }
  return files;
}

function walk(dir: string): string[] {
  const files: string[] = [];
  for (const entry of readdirSync(dir)) {
    const path = join(dir, entry);
    if (statSync(path).isDirectory()) files.push(...walk(path));
    else files.push(path);
  }
  return files;
}

/** Tokens of a Berry program, ignoring comments, layout, and `map["key"]` vs `map.key`. */
export function semanticTokens(source: string): string[] {
  return expandUninitializedVars(memberIndexToDot(tokenize(source)));
}

function tokenize(source: string): string[] {
  const tokens: string[] = [];
  let index = 0;
  while (index < source.length) {
    const char = source[index] ?? "";
    if (char === " " || char === "\t" || char === "\r" || char === "\n") {
      index += 1;
      continue;
    }
    if (char === "#") {
      while (index < source.length && source[index] !== "\n") index += 1;
      continue;
    }
    if (char === '"' || char === "'") {
      const quote = char;
      let end = index + 1;
      while (end < source.length) {
        if (source[end] === "\\") {
          end += 2;
          continue;
        }
        end += 1;
        if (source[end - 1] === quote) break;
      }
      tokens.push(source.slice(index, end));
      index = end;
      continue;
    }
    const operator = OPERATORS.find((item) => source.startsWith(item, index));
    if (operator) {
      tokens.push(operator);
      index += operator.length;
      continue;
    }
    if ("()[]{}.,:+-*/%!<>=".includes(char)) {
      tokens.push(char);
      index += 1;
      continue;
    }
    if (/[A-Za-z_]/.test(char) || /[0-9]/.test(char)) {
      const pattern = /[A-Za-z_]/.test(char) ? /[A-Za-z0-9_]/ : /[0-9]/;
      let end = index + 1;
      while (end < source.length && pattern.test(source[end] ?? "")) end += 1;
      tokens.push(source.slice(index, end));
      index = end;
      continue;
    }
    throw new Error(`Unexpected ${JSON.stringify(char)} while tokenizing Berry`);
  }
  return tokens;
}

function memberIndexToDot(tokens: readonly string[]): string[] {
  const normalized: string[] = [];
  for (let index = 0; index < tokens.length; index += 1) {
    const key = tokens[index + 1];
    if (
      tokens[index] === "[" &&
      tokens[index + 2] === "]" &&
      key !== undefined &&
      /^"(?:[A-Za-z_][A-Za-z0-9_]*)"$/.test(key)
    ) {
      normalized.push(".", key.slice(1, -1));
      index += 2;
      continue;
    }
    normalized.push(tokens[index] ?? "");
  }
  return normalized;
}

function expandUninitializedVars(tokens: readonly string[]): string[] {
  const expanded: string[] = [];
  let index = 0;
  while (index < tokens.length) {
    const name = tokens[index + 1];
    if (tokens[index] === "var" && isIdentifier(name)) {
      const names = [name];
      let cursor = index + 2;
      while (tokens[cursor] === "," && isIdentifier(tokens[cursor + 1])) {
        names.push(tokens[cursor + 1] ?? "");
        cursor += 2;
      }
      for (const binding of names) expanded.push("var", binding);
      index = cursor;
      continue;
    }
    expanded.push(tokens[index] ?? "");
    index += 1;
  }
  return expanded;
}

function isIdentifier(token: string | undefined): token is string {
  return token !== undefined && /^[A-Za-z_][A-Za-z0-9_]*$/.test(token);
}

function tokenDiff(expected: readonly string[], actual: readonly string[]): string {
  const length = Math.max(expected.length, actual.length);
  for (let index = 0; index < length; index += 1) {
    if (expected[index] === actual[index]) continue;
    const from = Math.max(0, index - 10);
    const to = Math.min(length, index + 10);
    const mark = (tokens: readonly string[]) =>
      tokens
        .slice(from, to)
        .map((token, offset) => (from + offset === index ? `>>${token ?? "∅"}<<` : (token ?? "∅")))
        .join(" ");
    return [
      `token ${index}`,
      `  berry: ${mark(expected)}`,
      `  ts:    ${mark(actual)}`,
    ].join("\n");
  }
  return "";
}

const entry = process.argv[1];
if (entry && import.meta.url === pathToFileURL(entry).href) {
  process.exit(main());
}
