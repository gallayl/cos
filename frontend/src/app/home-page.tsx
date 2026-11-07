import { createComponent, NestedRouteLink, Shade } from "@furystack/shades";
import {
  Card,
  CardContent,
  CardHeader,
  PageContainer,
  PageHeader,
  cssVariableTheme,
} from "@furystack/shades-common-components";

const features = [
  {
    path: "/editor",
    title: "Editor",
    description: "Edit device files and compile TypeScript to Berry.",
  },
  {
    path: "/console",
    title: "Console",
    description: "Send commands and follow device logs.",
  },
] as const;

/** Landing page that links to the main features. */
export const HomePage = Shade({
  customElementName: "cos-home-page",
  css: {
    display: "block",
    width: "100%",
    height: "100%",
    minHeight: "0",
    background: cssVariableTheme.background.default,
    color: cssVariableTheme.text.primary,
    "& .cards": {
      display: "grid",
      gridTemplateColumns: "repeat(auto-fit, minmax(16rem, 1fr))",
      gap: cssVariableTheme.spacing.lg,
    },
    "& nested-route-link": {
      display: "block",
      height: "100%",
      color: "inherit",
      textDecoration: "none",
    },
    "& shade-card": {
      height: "100%",
    },
  },
  render: () => (
    <PageContainer maxWidth="48rem" centered padding={cssVariableTheme.spacing.xl} gap={cssVariableTheme.spacing.lg}>
      <PageHeader title="COS" description="Choose a feature to open." />
      <div className="cards">
        {features.map((feature) => (
          <NestedRouteLink path={feature.path}>
            <Card clickable elevation={2}>
              <CardHeader title={feature.title} />
              <CardContent>
                <span>{feature.description}</span>
              </CardContent>
            </Card>
          </NestedRouteLink>
        ))}
      </div>
    </PageContainer>
  ),
});
