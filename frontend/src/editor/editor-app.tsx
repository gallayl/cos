import { createComponent, Shade } from "@furystack/shades";
import { cssVariableTheme } from "@furystack/shades-common-components";

import { astToBerry } from "../ide/ast-to-berry";
import { CosFilesError, describeCosError, writeTextFile } from "../ide/files/cos-files";
import { canOpenPath } from "../ide/files/paths";
import {
  isAbortError,
  pickTextFile,
  saveTextFile,
  supportsFilePicker,
  writeTextToHandle,
} from "../ide/files/local-files";
import { getAst } from "../ide/get-ast";
import "../ide/monaco-languages";
import "../ide/register-cos-sdk";
import { CompileOutput } from "./compile-output";
import { DeviceAccess } from "./device-access";
import {
  berryPlan,
  canCreateBerry,
  confirmDiscard,
  createScratchDocument,
  documentFromFile,
  formatDiagnostics,
  isDirty,
  lineCountLabel,
  locationText,
  replaceText,
  savePlan,
  type EditorDocument,
} from "./editor-document";
import { EditorStatusBar } from "./editor-status-bar";
import { EditorToolbar } from "./editor-toolbar";
import { FileBrowser } from "./file-browser";
import { FileCache } from "./file-cache";
import { SourceEditor } from "./source-editor";

const textFileAccept =
  ".ts,.tsx,.mts,.cts,.js,.jsx,.mjs,.cjs,.be,.berry,.txt,.json,.css,.html,.htm,.md,.xml,.yml,.yaml,.c,.h,.cpp,.hpp,.log";

/** COS TypeScript editor shell. */
export const EditorApp = Shade({
  customElementName: "shades-editor-app",
  css: {
    display: "flex",
    flexDirection: "column",
    width: "100%",
    height: "100%",
    minHeight: "0",
    background: cssVariableTheme.background.default,
    color: cssVariableTheme.text.primary,
    fontFamily: cssVariableTheme.typography.fontFamily,
    "& .workspace": {
      flex: "1",
      minHeight: "0",
      display: "flex",
    },
    "& .editor-column": {
      flex: "1",
      minWidth: "0",
      minHeight: "0",
      display: "flex",
      flexDirection: "column",
    },
    "& .file-input": {
      display: "none",
    },
  },
  render: ({ useState, useDisposable, useRef, injector }) => {
    const [document, setDocument] = useState<EditorDocument>("document", createScratchDocument());
    const [output, setOutput] = useState("output", "");
    const [status, setStatus] = useState("status", "TypeScript");
    const [localHandle, setLocalHandle] = useState<FileSystemFileHandle | null>("localHandle", null);
    const [revision, setRevision] = useState("revision", 0);
    const [directory, setDirectory] = useState("directory", "/flash");
    const fileInput = useRef<HTMLInputElement>("file-input");
    const live = useDisposable("live-document", () => ({
      document,
      localHandle,
      [Symbol.dispose]() {},
    }));
    live.document = document;
    live.localHandle = localHandle;

    const access = injector.get(DeviceAccess);
    const cache = injector.get(FileCache);
    const dirty = isDirty(document);

    const discard = () => confirmDiscard(dirty, (message) => window.confirm(message));

    const noteFailure = (error: unknown) => {
      if (error instanceof CosFilesError && error.status === 401) {
        access.markUnauthorized();
      }
      setStatus(describeCosError(error));
    };

    const replaceDocument = (next: EditorDocument, handle: FileSystemFileHandle | null, nextStatus: string) => {
      setDocument(next);
      setLocalHandle(handle);
      setOutput("");
      setRevision(revision + 1);
      setStatus(nextStatus);
    };

    const openDeviceFile = async (path: string) => {
      if (!canOpenPath(path)) {
        setStatus("That file type cannot be opened as text");
        return;
      }
      if (!discard()) {
        return;
      }
      setStatus(`Opening ${path}`);
      try {
        const authorization = access.authorization.getValue();
        const text = await cache.contents.get(path, authorization);
        replaceDocument(documentFromFile("device", path, text), null, path);
      } catch (error) {
        noteFailure(error);
      }
    };

    const openLocalFile = (name: string, text: string, handle: FileSystemFileHandle | null) => {
      if (!canOpenPath(name)) {
        setStatus("That file type cannot be opened as text");
        return;
      }
      replaceDocument(documentFromFile("local", name, text), handle, name);
    };

    const browseLocal = async () => {
      if (!discard()) {
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
        openLocalFile(picked.name, picked.text, picked.handle);
      } catch (error) {
        if (!isAbortError(error)) {
          setStatus(describeCosError(error));
        }
      }
    };

    const save = async () => {
      const plan = savePlan(document, localHandle !== null);
      const written = document.text;
      const authorization = access.authorization.getValue();
      const keepSaved = (patch: Partial<EditorDocument> = {}) => {
        setDocument({ ...live.document, ...patch, savedText: written });
      };

      if (plan.kind === "device") {
        setStatus(`Saving ${plan.path}`);
        try {
          await writeTextFile(plan.path, written, { authorization });
          keepSaved();
          cache.rememberFile(plan.path, authorization, written);
          cache.refreshDirectory(directory, authorization);
          setStatus(`Saved ${plan.path}`);
        } catch (error) {
          noteFailure(error);
        }
        return;
      }

      if (plan.kind === "handle" && localHandle) {
        setStatus(`Saving ${document.path}`);
        try {
          await writeTextToHandle(localHandle, written);
          keepSaved();
          setStatus(`Saved ${document.path}`);
        } catch (error) {
          if (!isAbortError(error)) {
            setStatus(describeCosError(error));
          }
        }
        return;
      }

      const suggested = plan.kind === "save-as" ? plan.suggestedName : document.path || "script.ts";
      try {
        const handle = await saveTextFile(suggested, written);
        if (handle) {
          keepSaved({ origin: "local", path: handle.name });
          setLocalHandle(handle);
        } else {
          keepSaved();
        }
        setStatus(`Saved ${handle?.name ?? suggested}`);
      } catch (error) {
        if (!isAbortError(error)) {
          setStatus(describeCosError(error));
        }
      }
    };

    const showCompileResult = () => {
      const result = astToBerry(getAst(document.text));
      if (result.ok) {
        setOutput(result.code);
        setStatus("Compiled");
        return result;
      }
      setOutput(formatDiagnostics(result.diagnostics));
      setStatus(`${result.diagnostics.length} error(s)`);
      return result;
    };

    const createBerry = async () => {
      const plan = berryPlan(document);
      if (plan.kind === "unsupported") {
        setStatus("Open a TypeScript file to create a Berry script");
        return;
      }
      const result = showCompileResult();
      if (!result.ok) {
        return;
      }
      if (plan.kind === "device") {
        try {
          const authorization = access.authorization.getValue();
          await writeTextFile(plan.path, result.code, { authorization });
          cache.refreshDirectory(directory, authorization);
          setStatus(`Wrote ${plan.path}`);
        } catch (error) {
          noteFailure(error);
        }
        return;
      }
      try {
        await saveTextFile(plan.suggestedName, result.code);
        setStatus(`Created ${plan.suggestedName}`);
      } catch (error) {
        if (!isAbortError(error)) {
          setStatus(describeCosError(error));
        }
      }
    };

    const reset = () => {
      if (!discard()) {
        return;
      }
      replaceDocument(createScratchDocument(), null, "TypeScript");
    };

    const onPickedFile = async (event: Event) => {
      const input = event.target as HTMLInputElement;
      const file = input.files?.[0];
      input.value = "";
      if (!file) {
        return;
      }
      openLocalFile(file.name, await file.text(), null);
    };

    return (
      <>
        <EditorToolbar
          canCreateBerry={canCreateBerry(document.language)}
          onOpen={() => void browseLocal()}
          onSave={() => void save()}
          onCreateBerry={() => void createBerry()}
          onCompile={showCompileResult}
          onReset={reset}
        />
        <div className="workspace">
          <FileBrowser
            directory={directory}
            activePath={document.path}
            onDirectoryChange={setDirectory}
            onOpenFile={(path) => void openDeviceFile(path)}
          />
          <div className="editor-column">
            <SourceEditor
              value={document.text}
              language={document.language}
              revision={revision}
              onValueChange={(text) => setDocument(replaceText(document, text))}
            />
            {output ? <CompileOutput text={output} /> : null}
          </div>
        </div>
        <EditorStatusBar
          status={status}
          location={locationText(document.path, dirty)}
          lines={lineCountLabel(document.text)}
        />
        <input
          ref={fileInput}
          className="file-input"
          type="file"
          accept={textFileAccept}
          onchange={(event) => void onPickedFile(event as Event)}
        />
      </>
    );
  },
});
