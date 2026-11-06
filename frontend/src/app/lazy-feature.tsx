import { createComponent, LazyLoad, Shade } from "@furystack/shades";
import { Alert, Button, Loader, cssVariableTheme } from "@furystack/shades-common-components";

export interface LazyFeatureProps {
  /** Loads the feature element the first time this route is shown. */
  load: () => Promise<JSX.Element>;
}

/** Defers a route's module until the route is visited. */
export const LazyFeature = Shade<LazyFeatureProps>({
  customElementName: "cos-lazy-feature",
  css: {
    display: "block",
    width: "100%",
    height: "100%",
    minHeight: "0",
    "& lazy-load": {
      display: "block",
      width: "100%",
      height: "100%",
      minHeight: "0",
    },
    "& .feature-loader, & .feature-error": {
      display: "flex",
      flexDirection: "column",
      alignItems: "center",
      justifyContent: "center",
      gap: cssVariableTheme.spacing.md,
      width: "100%",
      height: "100%",
      padding: cssVariableTheme.spacing.lg,
    },
    "& shade-loader": {
      width: "3rem",
      height: "3rem",
    },
    "& shade-alert": {
      maxWidth: "32rem",
    },
  },
  render: ({ props }) => (
    <LazyLoad
      loader={
        <div className="feature-loader">
          <Loader />
        </div>
      }
      component={props.load}
      error={(error, retry) => (
        <div className="feature-error">
          <Alert severity="error" title="Could not open this feature">
            {error instanceof Error ? error.message : "The feature failed to load."}
          </Alert>
          <Button type="button" variant="outlined" onclick={() => void retry()}>
            Retry
          </Button>
        </div>
      )}
    />
  ),
});
