import { createComponent, Shade } from "@furystack/shades";
import { MonacoEditor } from "@furystack/shades-monaco";
import { cssVariableTheme } from "@furystack/shades-common-components";
import { editor as monacoEditor } from "monaco-editor";

import { syncModels, type SyncableModel } from "./editor-document";

export interface SourceEditorProps {
  value: string;
  language: string;
  /** Bumped when the buffer is replaced from outside Monaco. */
  revision: number;
  onValueChange: (value: string) => void;
}

/** Monaco editor. External replacements sync in-memory models without resetting the cursor on each keystroke. */
export const SourceEditor = Shade<SourceEditorProps>({
  customElementName: "cos-source-editor",
  css: {
    display: "block",
    flex: "1",
    minHeight: "0",
    minWidth: "0",
    padding: cssVariableTheme.spacing.md,
    "& > *": {
      display: "block",
      width: "100%",
      height: "100%",
      minHeight: "0",
      minWidth: "0",
    },
  },
  render: ({ props, useDisposable }) => {
    const onChange = useDisposable("on-change", () => ({
      current: props.onValueChange,
      [Symbol.dispose]() {},
    }));
    onChange.current = props.onValueChange;

    useDisposable("editor-changes", () => watchEditorText((text) => onChange.current(text)));

    useDisposable(
      "sync-editor",
      () => {
        const text = props.value;
        const languageId = props.language;
        queueMicrotask(() => syncMonacoModels(text, languageId));
        return { [Symbol.dispose]() {} };
      },
      [props.revision, props.language],
    );

    return (
      <MonacoEditor
        value={props.value}
        options={{
          language: props.language,
          theme: "vs-dark",
          automaticLayout: true,
          minimap: { enabled: false },
          fontSize: 14,
          scrollBeyondLastLine: false,
          wordWrap: "on",
        }}
      />
    );
  },
});

/** Report every model edit. Monaco's own value callback keeps the first listener and skips same-length changes. */
function watchEditorText(onText: (text: string) => void): { [Symbol.dispose](): void } {
  const seen = new Set<monacoEditor.ICodeEditor>();
  const subscriptions: { dispose(): void }[] = [];

  const attach = (instance: monacoEditor.ICodeEditor) => {
    if (seen.has(instance)) {
      return;
    }
    seen.add(instance);
    subscriptions.push(
      instance.onDidChangeModelContent(() => {
        onText(instance.getValue());
      }),
    );
  };

  for (const instance of monacoEditor.getEditors()) {
    attach(instance);
  }
  const created = monacoEditor.onDidCreateEditor(attach);

  return {
    [Symbol.dispose]() {
      created.dispose();
      for (const subscription of subscriptions) {
        subscription.dispose();
      }
    },
  };
}

function syncMonacoModels(text: string, languageId: string): void {
  const models: SyncableModel[] = monacoEditor.getModels().map((model) => ({
    scheme: model.uri.scheme,
    getLanguageId: () => model.getLanguageId(),
    getValue: () => model.getValue(),
    setLanguage: (nextLanguage: string) => monacoEditor.setModelLanguage(model, nextLanguage),
    setValue: (nextValue: string) => model.setValue(nextValue),
  }));
  syncModels(models, text, languageId);
}
