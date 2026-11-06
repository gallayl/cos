import { createComponent, Shade } from "@furystack/shades";
import { cssVariableTheme } from "@furystack/shades-common-components";

import { fileEntryLabel } from "./editor-document";

export interface FileEntryProps {
  name: string;
  path: string;
  isDirectory: boolean;
  active: boolean;
  onOpen: (path: string, isDirectory: boolean) => void;
}

/** One directory row. Directories use the info color; the open file uses the selected background. */
export const FileEntry = Shade<FileEntryProps>({
  customElementName: "cos-file-entry",
  elementBase: HTMLButtonElement,
  elementBaseName: "button",
  css: {
    display: "block",
    width: "100%",
    textAlign: "left",
    padding: `${cssVariableTheme.spacing.xs} ${cssVariableTheme.spacing.sm}`,
    border: "none",
    borderRadius: cssVariableTheme.shape.borderRadius.sm,
    background: "transparent",
    color: cssVariableTheme.text.primary,
    fontFamily: "ui-monospace, SFMono-Regular, Menlo, Consolas, monospace",
    fontSize: cssVariableTheme.typography.fontSize.sm,
    cursor: "pointer",
    "&[data-directory]": {
      color: cssVariableTheme.palette.info.main,
    },
    "&[data-active]": {
      background: cssVariableTheme.action.selectedBackground,
    },
    "&:hover": {
      background: cssVariableTheme.action.hoverBackground,
    },
  },
  render: ({ props, useHostProps }) => {
    useHostProps({
      type: "button",
      title: props.path,
      onclick: () => props.onOpen(props.path, props.isDirectory),
      ...(props.isDirectory ? { "data-directory": "" } : {}),
      ...(props.active ? { "data-active": "" } : {}),
    });
    return <>{fileEntryLabel(props.name, props.isDirectory)}</>;
  },
});
