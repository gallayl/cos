import { createComponent, Shade } from "@furystack/shades";
import { cssVariableTheme } from "@furystack/shades-common-components";

import { entryNameError } from "./file-tree";

export interface FileNameFieldProps {
  initialValue: string;
  placeholder: string;
  selectFrom: number;
  selectTo: number;
  validate: (name: string) => string | null;
  onInvalid: (message: string) => void;
  onCommit: (name: string) => void;
  onCancel: () => void;
}

/** Single-line name editor used for new files, new folders, and rename. */
export const FileNameField = Shade<FileNameFieldProps>({
  customElementName: "cos-file-name-field",
  css: {
    display: "block",
    minWidth: "0",
    flex: "1",
    "& input": {
      width: "100%",
      boxSizing: "border-box",
      border: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.palette.primary.main}`,
      borderRadius: cssVariableTheme.shape.borderRadius.sm,
      background: cssVariableTheme.background.default,
      color: cssVariableTheme.text.primary,
      font: "inherit",
      padding: `0 ${cssVariableTheme.spacing.xs}`,
    },
  },
  render: ({ props, useRef, useDisposable }) => {
    const input = useRef<HTMLInputElement>("input");
    const once = useDisposable("once", () => ({ done: false, [Symbol.dispose]() {} }));

    const finish = (value: string, cancel: boolean) => {
      if (once.done) {
        return;
      }
      if (cancel || value.trim() === "") {
        once.done = true;
        props.onCancel();
        return;
      }
      const error = entryNameError(value) ?? props.validate(value);
      if (error) {
        props.onInvalid(error);
        input.current?.focus();
        return;
      }
      once.done = true;
      props.onCommit(value);
    };

    useDisposable("focus", () => {
      queueMicrotask(() => {
        const field = input.current;
        if (!field) {
          return;
        }
        field.value = props.initialValue;
        field.focus();
        field.setSelectionRange(props.selectFrom, props.selectTo);
      });
      return { [Symbol.dispose]() {} };
    });

    return (
      <input
        ref={input}
        type="text"
        placeholder={props.placeholder}
        onmousedown={(event) => event.stopPropagation()}
        onclick={(event) => event.stopPropagation()}
        ondblclick={(event) => event.stopPropagation()}
        onkeydown={(event) => {
          event.stopPropagation();
          if (event.key === "Enter") {
            event.preventDefault();
            finish((event.currentTarget as HTMLInputElement).value, false);
          } else if (event.key === "Escape") {
            event.preventDefault();
            finish((event.currentTarget as HTMLInputElement).value, true);
          }
        }}
        onblur={(event) => finish((event.currentTarget as HTMLInputElement).value, false)}
      />
    );
  },
});
