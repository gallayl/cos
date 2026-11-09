import { createComponent, NestedRouter, Shade } from '@furystack/shades'
import { cssVariableTheme } from '@furystack/shades-common-components'

import { NotFoundPage } from './not-found-page.js'
import { routes } from './routes.js'

/** Application shell. Feature pages are chosen by the URL. */
export const App = Shade({
  customElementName: 'cos-app',
  css: {
    display: 'block',
    width: '100%',
    height: '100%',
    minHeight: '0',
    background: cssVariableTheme.background.default,
    color: cssVariableTheme.text.primary,
    fontFamily: cssVariableTheme.typography.fontFamily,
    '& shade-nested-router': {
      display: 'block',
      width: '100%',
      height: '100%',
      minHeight: '0',
    },
  },
  render: () => <NestedRouter routes={routes} notFound={<NotFoundPage />} />,
})
