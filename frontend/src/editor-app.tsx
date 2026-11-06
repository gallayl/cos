import { Shade, createComponent } from "@furystack/shades";
import { MonacoEditor } from "@furystack/shades-monaco";
import {
  ThemeProviderService,
  defaultDarkTheme,
} from "@furystack/shades-common-components";

import { typescriptDefaults } from "monaco-editor/languages/features/typescript/register";

import sdkTypes from "./cos-sdk/ui.d.ts?raw" with { type: "text" };
import { astToBerry } from "./ide/ast-to-berry";
import { getAst } from "./ide/get-ast";

typescriptDefaults.addExtraLib(sdkTypes, "file:///cos-sdk/ui.d.ts");
const initialCode = `export default class Counter {
  private ticks = 0;

  setup(content: unknown, width: number, height: number): void {
    ui.timer(1000, () => this.tick());
  }

  private tick(): void {
    this.ticks += 1;
    log("ticks: " + this.ticks);
  }
}
`;

export const EditorApp = Shade({
  customElementName: "shades-editor-app",

  css: {
    display: "block",
    width: "100%",
    height: "100%",

    "& .app-shell": {
      display: "flex",
      flexDirection: "column",
      height: "100%",
      minHeight: "0",
      backgroundColor: "#111318",
      color: "#e5e7eb",
      fontFamily: "Inter, system-ui, sans-serif",
    },

    "& .toolbar": {
      height: "52px",
      minHeight: "52px",
      display: "flex",
      alignItems: "center",
      justifyContent: "space-between",
      padding: "0 16px",
      borderBottom: "1px solid #292d35",
      backgroundColor: "#181b21",
    },

    "& .brand": {
      fontSize: "14px",
      fontWeight: "600",
    },

    "& .toolbar-actions": {
      display: "flex",
      gap: "8px",
    },

    "& .toolbar button": {
      padding: "6px 12px",
      border: "1px solid #3b414d",
      borderRadius: "5px",
      backgroundColor: "#252a33",
      color: "#e5e7eb",
      cursor: "pointer",
    },

    "& .toolbar button:hover": {
      backgroundColor: "#343b48",
    },

    "& .editor-area": {
      flex: "1",
      minHeight: "0",
      padding: "10px",
    },

    "& .editor-area > *": {
      display: "block",
      width: "100%",
      height: "100%",
      minHeight: "0",
      minWidth: "0",
    },

    "& .output": {
      height: "220px",
      minHeight: "120px",
      margin: "0 10px 10px",
      padding: "10px 12px",
      overflow: "auto",
      border: "1px solid #292d35",
      borderRadius: "6px",
      backgroundColor: "#0d0f13",
      color: "#d5dbe6",
      fontFamily: "ui-monospace, SFMono-Regular, Menlo, Consolas, monospace",
      fontSize: "12px",
      lineHeight: "1.45",
      whiteSpace: "pre",
    },

    "& .statusbar": {
      height: "26px",
      minHeight: "26px",
      display: "flex",
      alignItems: "center",
      justifyContent: "space-between",
      padding: "0 12px",
      backgroundColor: "#181b21",
      borderTop: "1px solid #292d35",
      fontSize: "11px",
      color: "#8b929e",
    },
  },

  render: ({ useState, injector }) => {
    const [value, setValue] = useState("source", initialCode);
    const [output, setOutput] = useState("output", "");
    const [status, setStatus] = useState("status", "TypeScript");

    const theme = injector.get(ThemeProviderService);
    theme.setAssignedTheme(defaultDarkTheme);

    const compile = () => {
      const result = astToBerry(getAst(value));
      if (result.ok) {
        setOutput(result.code);
        setStatus("Compiled");
        return;
      }
      setOutput(
        result.diagnostics
          .map((diagnostic) => `${diagnostic.line}:${diagnostic.column} ${diagnostic.message}`)
          .join("\n"),
      );
      setStatus(`${result.diagnostics.length} error(s)`);
    };

    return (
      <div className="app-shell">
        <header className="toolbar">
          <div className="brand">COS TypeScript</div>

          <div className="toolbar-actions">
            <button type="button" onclick={compile}>
              Compile
            </button>
            <button
              type="button"
              onclick={() => {
                setValue(initialCode);
                setOutput("");
                setStatus("TypeScript");
              }}
            >
              Reset
            </button>
          </div>
        </header>

        <main className="editor-area">
          <MonacoEditor
            value={value}
            onValueChange={(nextValue) => setValue(nextValue)}
            options={{
              language: "typescript",
              theme: "vs-dark",
              automaticLayout: true,
              minimap: {
                enabled: false,
              },
              fontSize: 14,
              scrollBeyondLastLine: false,
              wordWrap: "on",
            }}
          />
        </main>

        {output ? <pre className="output">{output}</pre> : null}

        <footer className="statusbar">
          <span>{status}</span>
          <span>{value.split("\n").length} lines</span>
        </footer>
      </div>
    );
  },
});
