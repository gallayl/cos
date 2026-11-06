import { createComponent, Shade } from "@furystack/shades";
import { Skeleton, cssVariableTheme } from "@furystack/shades-common-components";

/** Placeholder rows while a directory listing is loading. */
export const DirectorySkeleton = Shade({
  customElementName: "cos-directory-skeleton",
  css: {
    display: "flex",
    flexDirection: "column",
    gap: cssVariableTheme.spacing.sm,
    padding: cssVariableTheme.spacing.sm,
    "& shade-skeleton": {
      display: "block",
      height: "1.25rem",
    },
  },
  render: () => (
    <div>
      <Skeleton delay={0} />
      <Skeleton delay={0} />
      <Skeleton delay={0} />
      <Skeleton delay={0} />
      <Skeleton delay={0} />
    </div>
  ),
});
