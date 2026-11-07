import { createComponent, Shade } from "@furystack/shades";
import { Paper, cssVariableTheme } from "@furystack/shades-common-components";

/** Compiler output or diagnostic text under the editor. */
export const CompileOutput = Shade<{ text: string }>({
  customElementName: "cos-compile-output",
  css: {
    display: "block",
    margin: `0 ${cssVariableTheme.spacing.md} ${cssVariableTheme.spacing.md}`,
    "& pre": {
      margin: "0",
      maxHeight: "10rem",
      overflow: "auto",
      whiteSpace: "pre",
      fontFamily: "ui-monospace, SFMono-Regular, Menlo, Consolas, monospace",
      fontSize: cssVariableTheme.typography.fontSize.sm,
      lineHeight: cssVariableTheme.typography.lineHeight.normal,
      color: cssVariableTheme.text.primary,
    },
  },
  render: ({ props }) => (
    <Paper elevation={0}>
      <pre>{props.text}</pre>
    </Paper>
  ),
});
