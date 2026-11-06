import {
  javascriptDefaults,
  ModuleKind,
  ScriptTarget,
  typescriptDefaults,
  type CompilerOptions,
} from "monaco-editor/languages/features/typescript/register";

import consoleTypes from "../cos-sdk/console.d.ts?raw" with { type: "text" };
import filesystemTypes from "../cos-sdk/filesystem.d.ts?raw" with { type: "text" };
import indexTypes from "../cos-sdk/index.d.ts?raw" with { type: "text" };
import languageTypes from "../cos-sdk/language.d.ts?raw" with { type: "text" };
import networkTypes from "../cos-sdk/network.d.ts?raw" with { type: "text" };
import uiTypes from "../cos-sdk/ui.d.ts?raw" with { type: "text" };

const sdkRoot = "file:///cos-sdk";

/** Declaration files passed to Monaco. Paths match the triple-slash references in `index.d.ts`. */
const cosLibs = [
  { content: languageTypes, filePath: `${sdkRoot}/language.d.ts` },
  { content: uiTypes, filePath: `${sdkRoot}/ui.d.ts` },
  { content: consoleTypes, filePath: `${sdkRoot}/console.d.ts` },
  { content: filesystemTypes, filePath: `${sdkRoot}/filesystem.d.ts` },
  { content: networkTypes, filePath: `${sdkRoot}/network.d.ts` },
  { content: indexTypes, filePath: `${sdkRoot}/index.d.ts` },
];

/**
 * TypeScript subset used by COS scripts.
 * `noLib` drops the DOM and ECMAScript default libraries, including `console.log`.
 */
const cosCompilerOptions: CompilerOptions = {
  allowNonTsExtensions: true,
  target: ScriptTarget.ESNext,
  module: ModuleKind.ESNext,
  noLib: true,
  strict: true,
};

typescriptDefaults.setCompilerOptions(cosCompilerOptions);
typescriptDefaults.setExtraLibs(cosLibs);

javascriptDefaults.setCompilerOptions({
  ...cosCompilerOptions,
  allowJs: true,
  checkJs: true,
});
javascriptDefaults.setDiagnosticsOptions({
  noSemanticValidation: false,
  noSyntaxValidation: false,
  onlyVisible: false,
});
javascriptDefaults.setExtraLibs(cosLibs);
