import { createComponent, NestedRouter, Shade } from '@furystack/shades'
import { NotyList, cssVariableTheme } from '@furystack/shades-common-components'

import { DeviceAccess } from '../editor/device-access.js'
import { DeviceSignIn } from '../editor/device-sign-in.js'
import { MemoryFootprint } from './memory-footprint.js'
import { NotFoundPage } from './not-found-page.js'
import { routes } from './routes.js'

/** Application shell. Feature pages are chosen by the URL. */
export const App = Shade({
  customElementName: 'cos-app',
  css: {
    display: 'flex',
    flexDirection: 'column',
    width: '100%',
    height: '100%',
    minHeight: '0',
    background: cssVariableTheme.background.default,
    color: cssVariableTheme.text.primary,
    fontFamily: cssVariableTheme.typography.fontFamily,
    '& .content': {
      flex: '1',
      minHeight: '0',
      display: 'flex',
      flexDirection: 'column',
    },
    '& .global-auth': {
      borderBottom: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
      background: cssVariableTheme.background.paper,
    },
    '& shade-nested-router': {
      display: 'block',
      width: '100%',
      height: '100%',
      minHeight: '0',
      flex: '1',
    },
    '& footer.app-footer': {
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between',
      gap: cssVariableTheme.spacing.md,
      minHeight: '1.75rem',
      padding: `0 ${cssVariableTheme.spacing.md}`,
      background: cssVariableTheme.background.paper,
      borderTop: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
      fontSize: cssVariableTheme.typography.fontSize.xs,
      color: cssVariableTheme.text.secondary,
    },
  },
  render: ({ injector, useObservable }) => {
    const access = injector.get(DeviceAccess)
    const [authorization] = useObservable('authorization', access.authorization)
    const [needsAuth] = useObservable('needs-auth', access.needsAuth)
    const showSignIn = !authorization || needsAuth

    return (
      <>
        <div className="content">
          {showSignIn ? (
            <div className="global-auth">
              <DeviceSignIn />
            </div>
          ) : null}
          <NestedRouter routes={routes} notFound={<NotFoundPage />} />
        </div>
        <footer className="app-footer">
          <span>COS</span>
          <MemoryFootprint />
        </footer>
        <NotyList />
      </>
    )
  },
})

