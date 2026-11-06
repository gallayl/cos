import { createComponent, defineNestedRoutes } from "@furystack/shades";

import { HomePage } from "./home-page.js";
import { LazyFeature } from "./lazy-feature.js";

const loadEditor = async () => {
  const { EditorApp } = await import("../editor/editor-app.js");
  return <EditorApp />;
};

const loadConsole = async () => {
  const { ConsolePage } = await import("../console/console-page.js");
  return <ConsolePage />;
};

/** Top-level application routes. Feature routes load their modules on visit. */
export const routes = defineNestedRoutes({
  "/editor": {
    meta: { title: "Editor" },
    component: () => <LazyFeature load={loadEditor} />,
  },
  "/console": {
    meta: { title: "Console" },
    component: () => <LazyFeature load={loadConsole} />,
  },
  "/": {
    meta: { title: "COS" },
    component: () => <HomePage />,
  },
});
