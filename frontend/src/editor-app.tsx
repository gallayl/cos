import { Shade, createComponent } from "@furystack/shades";
import { MonacoEditor } from "@furystack/shades-monaco";
import {
  ThemeProviderService,
  defaultDarkTheme,
} from "@furystack/shades-common-components";

import { typescriptDefaults } from "monaco-editor/languages/features/typescript/register";

import sdkTypes from "./cos-sdk/ui.d.ts?raw" with { type: "text" };

typescriptDefaults.addExtraLib(sdkTypes, "file:///cos-sdk/ui.d.ts");

const initialCode = `// Welcome to Shades + Monaco

import { Shade } from '@furystack/shades'

export const HelloWorld = Shade({
  customElementName: 'hello-world',

  render: () => (
    <div>
      Hello from FuryStack!
    </div>
  ),
})
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

    const theme = injector.get(ThemeProviderService);
    theme.setAssignedTheme(defaultDarkTheme);

    return (
      <div className="app-shell">
        <header className="toolbar">
          <div className="brand">Shades Playground</div>

          <button type="button" onclick={() => setValue(initialCode)}>
            Reset
          </button>
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

        <footer className="statusbar">
          <span>TypeScript</span>
          <span>{value.split("\n").length} lines</span>
        </footer>
      </div>
    );
  },
});
