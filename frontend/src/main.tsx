import "./style.css";

import { createInjector } from "@furystack/inject";
import { createComponent, initializeShadeRoot } from "@furystack/shades";

import { EditorApp } from "./editor-app.js";

const rootElement = document.querySelector<HTMLDivElement>("#root");

if (!rootElement) {
  throw new Error("Root element not found");
}

initializeShadeRoot({
  injector: createInjector(),
  rootElement,
  jsxElement: <EditorApp />,
});
