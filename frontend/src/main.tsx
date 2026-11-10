import './style.css'

import { createInjector } from '@furystack/inject'
import { createComponent, initializeShadeRoot } from '@furystack/shades'
import { ThemeProviderService, defaultDarkTheme } from '@furystack/shades-common-components'

import { App } from './app/app.js'

const rootElement = document.querySelector<HTMLDivElement>('#root')

if (!rootElement) {
  throw new Error('Root element not found')
}

const injector = createInjector()
injector.get(ThemeProviderService).setAssignedTheme(defaultDarkTheme)

initializeShadeRoot({
  injector,
  rootElement,
  jsxElement: <App />,
})
