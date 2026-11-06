import { createComponent, Shade, nestedNavigate } from "@furystack/shades";
import { Button, PageContainer, PageHeader, cssVariableTheme } from "@furystack/shades-common-components";

/** Placeholder for the device console. */
export const ConsolePage = Shade({
  customElementName: "cos-console-page",
  css: {
    display: "block",
    width: "100%",
    height: "100%",
    minHeight: "0",
    background: cssVariableTheme.background.default,
    color: cssVariableTheme.text.primary,
  },
  render: ({ injector }) => (
    <PageContainer maxWidth="40rem" centered padding={cssVariableTheme.spacing.xl} gap={cssVariableTheme.spacing.lg}>
      <PageHeader title="Console" description="The device console is not available yet." />
      <Button
        type="button"
        variant="outlined"
        style={{ alignSelf: "flex-start" }}
        onclick={() => nestedNavigate(injector, { path: "/" })}
      >
        Back to home
      </Button>
    </PageContainer>
  ),
});
