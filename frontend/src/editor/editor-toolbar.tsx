import { createComponent, Shade } from "@furystack/shades";
import { Button, ButtonGroup, Typography, cssVariableTheme } from "@furystack/shades-common-components";

export interface EditorToolbarProps {
  canCreateBerry: boolean;
  onOpen: () => void;
  onSave: () => void;
  onCreateBerry: () => void;
  onCompile: () => void;
  onReset: () => void;
}

/** Primary editor actions. */
export const EditorToolbar = Shade<EditorToolbarProps>({
  customElementName: "cos-editor-toolbar",
  css: {
    display: "block",
    width: "100%",
    "& header": {
      display: "flex",
      alignItems: "center",
      justifyContent: "space-between",
      gap: cssVariableTheme.spacing.md,
      minHeight: "3rem",
      padding: `${cssVariableTheme.spacing.sm} ${cssVariableTheme.spacing.md}`,
      background: cssVariableTheme.background.paper,
      borderBottom: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
    },
    "& shade-typography": {
      margin: "0",
    },
    "& shade-button-group": {
      flexWrap: "wrap",
      justifyContent: "flex-end",
    },
  },
  render: ({ props }) => (
    <header>
      <Typography variant="subtitle2">COS TypeScript</Typography>
      <ButtonGroup variant="outlined">
        <Button type="button" size="small" onclick={props.onOpen}>
          Open
        </Button>
        <Button type="button" size="small" onclick={props.onSave}>
          Save
        </Button>
        <Button
          type="button"
          size="small"
          onclick={props.onCreateBerry}
          aria-disabled={props.canCreateBerry ? "false" : "true"}
          style={
            props.canCreateBerry
              ? undefined
              : { opacity: cssVariableTheme.action.disabledOpacity, pointerEvents: "none" }
          }
        >
          Create Berry
        </Button>
        <Button type="button" size="small" onclick={props.onCompile}>
          Compile
        </Button>
        <Button type="button" size="small" onclick={props.onReset}>
          Reset
        </Button>
      </ButtonGroup>
    </header>
  ),
});
