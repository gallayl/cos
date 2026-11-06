import { createComponent, Shade } from "@furystack/shades";
import { MonacoEditor } from "@furystack/shades-monaco";
import { ThemeProviderService, defaultDarkTheme } from "@furystack/shades-common-components";
import { editor as monacoEditor } from "monaco-editor";

import { astToBerry } from "./ide/ast-to-berry";
import {
  CosFilesError,
  basicAuthorization,
  describeCosError,
  listDirectory,
  readTextFile,
  writeTextFile,
  type CosDirEntry,
} from "./ide/files/cos-files";
import {
  berryPathFor,
  canOpenPath,
  fileStem,
  isTypeScriptPath,
  joinPath,
  languageForPath,
  parentPath,
} from "./ide/files/paths";
import {
  isAbortError,
  pickTextFile,
  saveTextFile,
  supportsFilePicker,
  writeTextToHandle,
} from "./ide/files/local-files";
import { getAst } from "./ide/get-ast";
import "./ide/monaco-languages";
import "./ide/register-cos-sdk";

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

type Origin = "none" | "device" | "local";

interface EditorSession {
  listGeneration: number;
  authorization: string;
  text: string;
  language: string;
  [Symbol.dispose](): void;
}

function syncEditor(text: string, languageId: string): void {
  for (const model of monacoEditor.getModels()) {
    if (model.uri.scheme !== "inmemory") {
      continue;
    }
    if (model.getLanguageId() !== languageId) {
      monacoEditor.setModelLanguage(model, languageId);
    }
    if (model.getValue() !== text) {
      model.setValue(text);
    }
  }
}

function diagnosticsText(diagnostics: readonly { line: number; column: number; message: string }[]): string {
  return diagnostics.map((diagnostic) => `${diagnostic.line}:${diagnostic.column} ${diagnostic.message}`).join("\n");
}

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
      minHeight: "52px",
      display: "flex",
      alignItems: "center",
      justifyContent: "space-between",
      gap: "12px",
      padding: "8px 16px",
      borderBottom: "1px solid #292d35",
      backgroundColor: "#181b21",
    },

    "& .brand": {
      fontSize: "14px",
      fontWeight: "600",
    },

    "& .toolbar-actions": {
      display: "flex",
      flexWrap: "wrap",
      justifyContent: "flex-end",
      gap: "8px",
    },

    "& button": {
      padding: "6px 12px",
      border: "1px solid #3b414d",
      borderRadius: "5px",
      backgroundColor: "#252a33",
      color: "#e5e7eb",
      cursor: "pointer",
    },

    "& button:hover": {
      backgroundColor: "#343b48",
    },

    "& button:disabled": {
      opacity: "0.45",
      cursor: "default",
    },

    "& .workspace": {
      flex: "1",
      minHeight: "0",
      display: "flex",
    },

    "& .browser": {
      width: "240px",
      minWidth: "180px",
      display: "flex",
      flexDirection: "column",
      borderRight: "1px solid #292d35",
      backgroundColor: "#14171c",
    },

    "& .browser-section": {
      display: "flex",
      flexDirection: "column",
      gap: "6px",
      padding: "8px",
      borderBottom: "1px solid #292d35",
    },

    "& .browser-section input": {
      width: "100%",
      padding: "6px 8px",
      border: "1px solid #3b414d",
      borderRadius: "5px",
      backgroundColor: "#0d0f13",
      color: "#e5e7eb",
    },

    "& .roots": {
      display: "flex",
      gap: "6px",
    },

    "& .roots button": {
      flex: "1",
    },

    "& .browser-path": {
      padding: "8px 10px 0",
      fontSize: "12px",
      color: "#8b929e",
      wordBreak: "break-all",
    },

    "& .browser-error": {
      margin: "8px 10px 0",
      color: "#f0a8a8",
      fontSize: "12px",
    },

    "& .browser-list": {
      flex: "1",
      overflow: "auto",
      margin: "0",
      padding: "4px",
      listStyle: "none",
    },

    "& .browser-list button": {
      width: "100%",
      textAlign: "left",
      padding: "4px 8px",
      border: "none",
      backgroundColor: "transparent",
      font: "12px ui-monospace, SFMono-Regular, Menlo, Consolas, monospace",
    },

    "& .browser-list button.dir": {
      color: "#8eb6ff",
    },

    "& .browser-list button.active": {
      backgroundColor: "#252a33",
    },

    "& .editor-column": {
      flex: "1",
      minWidth: "0",
      minHeight: "0",
      display: "flex",
      flexDirection: "column",
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
      height: "180px",
      minHeight: "96px",
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
      gap: "12px",
      padding: "0 12px",
      backgroundColor: "#181b21",
      borderTop: "1px solid #292d35",
      fontSize: "11px",
      color: "#8b929e",
    },

    "& .statusbar span": {
      overflow: "hidden",
      textOverflow: "ellipsis",
      whiteSpace: "nowrap",
    },

    "& .file-input": {
      display: "none",
    },
  },

  render: ({ useState, useDisposable, useRef, injector }) => {
    const [value, setValue] = useState("source", initialCode);
    const [saved, setSaved] = useState("saved", initialCode);
    const [output, setOutput] = useState("output", "");
    const [status, setStatus] = useState("status", "TypeScript");
    const [language, setLanguage] = useState("language", "typescript");
    const [origin, setOrigin] = useState<Origin>("origin", "none");
    const [filePath, setFilePath] = useState("filePath", "");
    const [localHandle, setLocalHandle] = useState<FileSystemFileHandle | null>("localHandle", null);
    const [generation, setGeneration] = useState("generation", 0);
    const [directory, setDirectory] = useState("directory", "/flash");
    const [entries, setEntries] = useState<CosDirEntry[]>("entries", []);
    const [browserError, setBrowserError] = useState("browserError", "");
    const [needsAuth, setNeedsAuth] = useState("needsAuth", false);
    const [username, setUsername] = useState("username", "");
    const [password, setPassword] = useState("password", "");

    const fileInput = useRef<HTMLInputElement>("file-input");
    const session = useDisposable("session", (): EditorSession => {
      return {
        listGeneration: 0,
        authorization: "",
        text: initialCode,
        language: "typescript",
        [Symbol.dispose]() {},
      };
    });
    session.text = value;
    session.language = language;

    useDisposable(
      "sync-editor",
      () => {
        const text = session.text;
        const languageId = session.language;
        queueMicrotask(() => syncEditor(text, languageId));
        return { [Symbol.dispose]() {} };
      },
      [generation, language],
    );

    const theme = injector.get(ThemeProviderService);
    theme.setAssignedTheme(defaultDarkTheme);

    const dirty = value !== saved;
    const parent = parentPath(directory);

    const noteFailure = (error: unknown) => {
      if (error instanceof CosFilesError && error.status === 401) {
        setNeedsAuth(true);
      }
      setStatus(describeCosError(error));
    };

    const confirmDiscard = () => {
      if (!dirty) {
        return true;
      }
      return window.confirm("Discard unsaved changes?");
    };

    const openDocument = (nextOrigin: Origin, path: string, text: string, handle: FileSystemFileHandle | null) => {
      setOrigin(nextOrigin);
      setFilePath(path);
      setLocalHandle(handle);
      setValue(text);
      setSaved(text);
      setLanguage(languageForPath(path));
      setOutput("");
      setGeneration(generation + 1);
      setStatus(path);
    };

    const loadDirectory = async (path: string) => {
      const requestId = ++session.listGeneration;
      setDirectory(path);
      setBrowserError("");
      try {
        const listed = await listDirectory(path, { authorization: session.authorization });
        if (requestId !== session.listGeneration) {
          return;
        }
        setEntries(listed);
        setNeedsAuth(false);
      } catch (error) {
        if (requestId !== session.listGeneration) {
          return;
        }
        setEntries([]);
        setBrowserError(describeCosError(error));
        if (error instanceof CosFilesError && error.status === 401) {
          setNeedsAuth(true);
        }
      }
    };

    useDisposable("browse-root", () => {
      void loadDirectory("/flash");
      return { [Symbol.dispose]() {} };
    });

    const openDeviceFile = async (path: string) => {
      if (!canOpenPath(path)) {
        setStatus("That file type cannot be opened as text");
        return;
      }
      if (!confirmDiscard()) {
        return;
      }
      setStatus(`Opening ${path}`);
      try {
        const text = await readTextFile(path, { authorization: session.authorization });
        openDocument("device", path, text, null);
      } catch (error) {
        noteFailure(error);
      }
    };

    const openLocalFile = async (name: string, text: string, handle: FileSystemFileHandle | null) => {
      if (!canOpenPath(name)) {
        setStatus("That file type cannot be opened as text");
        return;
      }
      openDocument("local", name, text, handle);
    };

    const browseLocal = async () => {
      if (!confirmDiscard()) {
        return;
      }
      if (!supportsFilePicker()) {
        fileInput.current?.click();
        return;
      }
      try {
        const picked = await pickTextFile();
        if (!picked) {
          fileInput.current?.click();
          return;
        }
        await openLocalFile(picked.name, picked.text, picked.handle);
      } catch (error) {
        if (!isAbortError(error)) {
          setStatus(describeCosError(error));
        }
      }
    };

    const save = async () => {
      if (origin === "device" && filePath) {
        setStatus(`Saving ${filePath}`);
        try {
          await writeTextFile(filePath, value, { authorization: session.authorization });
          setSaved(value);
          setStatus(`Saved ${filePath}`);
          void loadDirectory(directory);
        } catch (error) {
          noteFailure(error);
        }
        return;
      }

      if (origin === "local" && localHandle) {
        setStatus(`Saving ${filePath}`);
        try {
          await writeTextToHandle(localHandle, value);
          setSaved(value);
          setStatus(`Saved ${filePath}`);
        } catch (error) {
          if (!isAbortError(error)) {
            setStatus(describeCosError(error));
          }
        }
        return;
      }

      const suggested = filePath || "script.ts";
      try {
        const handle = await saveTextFile(suggested, value);
        if (handle) {
          setOrigin("local");
          setLocalHandle(handle);
          setFilePath(handle.name);
        }
        setSaved(value);
        setStatus(`Saved ${handle?.name ?? suggested}`);
      } catch (error) {
        if (!isAbortError(error)) {
          setStatus(describeCosError(error));
        }
      }
    };

    const showCompileResult = () => {
      const result = astToBerry(getAst(value));
      if (result.ok) {
        setOutput(result.code);
        setStatus("Compiled");
        return result;
      }
      setOutput(diagnosticsText(result.diagnostics));
      setStatus(`${result.diagnostics.length} error(s)`);
      return result;
    };

    const createBerry = async () => {
      const sourceName = filePath || "script.ts";
      if (language !== "typescript") {
        setStatus("Open a TypeScript file to create a Berry script");
        return;
      }
      const result = showCompileResult();
      if (!result.ok) {
        return;
      }

      if (origin === "device" && isTypeScriptPath(filePath)) {
        const target = berryPathFor(filePath);
        if (!target) {
          return;
        }
        try {
          await writeTextFile(target, result.code, { authorization: session.authorization });
          setStatus(`Wrote ${target}`);
          void loadDirectory(directory);
        } catch (error) {
          noteFailure(error);
        }
        return;
      }

      const suggested = `${fileStem(sourceName)}.be`;
      try {
        await saveTextFile(suggested, result.code);
        setStatus(`Created ${suggested}`);
      } catch (error) {
        if (!isAbortError(error)) {
          setStatus(describeCosError(error));
        }
      }
    };

    const reset = () => {
      if (!confirmDiscard()) {
        return;
      }
      setOrigin("none");
      setFilePath("");
      setLocalHandle(null);
      setValue(initialCode);
      setSaved(initialCode);
      setLanguage("typescript");
      setOutput("");
      setGeneration(generation + 1);
      setStatus("TypeScript");
    };

    const signIn = () => {
      if (username.trim() === "") {
        setBrowserError("Enter the device username");
        return;
      }
      try {
        session.authorization = basicAuthorization(username, password);
      } catch {
        setBrowserError("Username and password must be ASCII");
        return;
      }
      void loadDirectory(directory);
    };

    const onPickedFile = async (event: Event) => {
      const input = event.target as HTMLInputElement;
      const file = input.files?.[0];
      input.value = "";
      if (!file) {
        return;
      }
      await openLocalFile(file.name, await file.text(), null);
    };

    const locationLabel = filePath || "unsaved buffer";

    return (
      <div className="app-shell">
        <header className="toolbar">
          <div className="brand">COS TypeScript</div>
          <div className="toolbar-actions">
            <button type="button" onclick={() => void browseLocal()}>
              Open
            </button>
            <button type="button" onclick={() => void save()}>
              Save
            </button>
            <button type="button" onclick={() => void createBerry()} disabled={language !== "typescript"}>
              Create Berry
            </button>
            <button type="button" onclick={showCompileResult}>
              Compile
            </button>
            <button type="button" onclick={reset}>
              Reset
            </button>
          </div>
        </header>

        <div className="workspace">
          <aside className="browser">
            {needsAuth ? (
              <div className="browser-section">
                <input
                  type="text"
                  placeholder="Username"
                  autocomplete="username"
                  value={username}
                  oninput={(event) => setUsername((event.target as HTMLInputElement).value)}
                />
                <input
                  type="password"
                  placeholder="Password"
                  autocomplete="current-password"
                  value={password}
                  onkeydown={(event) => {
                    if ((event as KeyboardEvent).key === "Enter") {
                      signIn();
                    }
                  }}
                  oninput={(event) => setPassword((event.target as HTMLInputElement).value)}
                />
                <button type="button" onclick={signIn}>
                  Sign in
                </button>
              </div>
            ) : null}

            <div className="browser-section roots">
              <button type="button" onclick={() => void loadDirectory("/flash")}>
                flash
              </button>
              <button type="button" onclick={() => void loadDirectory("/sdcard")}>
                sdcard
              </button>
            </div>

            <div className="browser-path">{directory}</div>
            {browserError ? <p className="browser-error">{browserError}</p> : null}

            <ul className="browser-list">
              {parent ? (
                <li>
                  <button type="button" className="dir" onclick={() => void loadDirectory(parent)}>
                    ..
                  </button>
                </li>
              ) : null}
              {entries.map((entry) => {
                const path = joinPath(directory, entry.name);
                const className = [entry.is_dir ? "dir" : "file", path === filePath ? "active" : ""]
                  .filter(Boolean)
                  .join(" ");
                return (
                  <li>
                    <button
                      type="button"
                      className={className}
                      onclick={() => {
                        if (entry.is_dir) {
                          void loadDirectory(path);
                          return;
                        }
                        void openDeviceFile(path);
                      }}
                    >
                      {entry.is_dir ? `${entry.name}/` : entry.name}
                    </button>
                  </li>
                );
              })}
            </ul>
          </aside>

          <div className="editor-column">
            <main className="editor-area">
              <MonacoEditor
                value={value}
                onValueChange={(nextValue) => setValue(nextValue)}
                options={{
                  language,
                  theme: "vs-dark",
                  automaticLayout: true,
                  minimap: { enabled: false },
                  fontSize: 14,
                  scrollBeyondLastLine: false,
                  wordWrap: "on",
                }}
              />
            </main>
            {output ? <pre className="output">{output}</pre> : null}
          </div>
        </div>

        <footer className="statusbar">
          <span>{status}</span>
          <span>
            {locationLabel}
            {dirty ? " · modified" : ""}
          </span>
          <span>{value.split("\n").length} lines</span>
        </footer>

        <input
          ref={fileInput}
          className="file-input"
          type="file"
          accept=".ts,.tsx,.mts,.cts,.js,.jsx,.mjs,.cjs,.be,.berry,.txt,.json,.css,.html,.htm,.md,.xml,.yml,.yaml,.c,.h,.cpp,.hpp,.log"
          onchange={(event) => void onPickedFile(event as Event)}
        />
      </div>
    );
  },
});
