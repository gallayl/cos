# Frontend instructions (Furystack Shades)

Applies to `frontend/**` (TypeScript/TSX/CSS). These conventions complement the root `AGENTS.md` (which remains authoritative for shared DoD/validation reporting).

## Stack and structure

- The frontend uses **Furystack Shades** and `@furystack/inject`. Firmware C/C++ conventions do not apply here.
- Keep feature areas focused and colocated; prefer small, readable modules over broad abstractions.

## Components and services

- Prefer `@furystack/shades-common-components` (`Button`, `Input`, `Alert`, `Loader`, theme helpers) before adding a custom control.
- Keep each Shade small. Split a screen into focused pieces (e.g. toolbar vs content vs access panels).
- Put fetching, caching, and session logic in an injectable service. Shades read state via `injector.get(...)` and stay primarily views.

```typescript
export const DeviceAccess = defineService({
  name: 'cos/DeviceAccess',
  lifetime: 'singleton',
  factory: ({ onDispose }) => {
    const service = createDeviceAccess()
    onDispose(() => service[Symbol.dispose]())
    return service
  },
})
```

## Pages and lazy loading

- Load feature routes through `LazyFeature` and a dynamic `import()`. Keep `App`, home, and not-found eager.

```tsx
const loadConsole = async () => {
  const { ConsolePage } = await import("../console/console-page.js");
  return <ConsolePage />;
};

"/console": {
  meta: { title: "Console" },
  component: () => <LazyFeature load={loadConsole} />,
},
```

## Theme and styling

- Color, spacing, type, and shape come from the active Shades theme (`ThemeProviderService`, `cssVariableTheme`). Use theme tokens inside Shade `css` blocks.
- `frontend/src/style.css` is only the full-height viewport shell. Put new appearance in the theme and in component `css` using theme tokens (avoid hard-coded styling when a token fits).
