import { createComponent, Shade } from '@furystack/shades'
import { cssVariableTheme } from '@furystack/shades-common-components'

import { DeviceSignIn } from './device-sign-in'
import { DeviceAccess } from './device-access'
import { FileTreeView } from './file-tree-view'

export interface FileBrowserProps {
  activePath: string
  onOpenFile: (path: string) => void
  onMoved: (from: string, to: string) => void
  onActivity: (message: string) => void
}

/** Device filesystem browser. Folders expand in place and accept dropped files. */
export const FileBrowser = Shade<FileBrowserProps>({
  customElementName: 'cos-file-browser',
  css: {
    display: 'flex',
    flexDirection: 'column',
    width: '16rem',
    minWidth: '11rem',
    height: '100%',
    minHeight: '0',
    background: cssVariableTheme.background.paper,
    borderRight: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
    '& cos-file-tree-view': {
      flex: '1',
      minHeight: '0',
    },
  },
  render: ({ props, injector, useObservable }) => {
    const access = injector.get(DeviceAccess)
    const [needsAuth] = useObservable('needs-auth', access.needsAuth)

    return (
      <>
        {needsAuth ? <DeviceSignIn /> : null}
        <FileTreeView
          activePath={props.activePath}
          onOpenFile={props.onOpenFile}
          onMoved={props.onMoved}
          onActivity={props.onActivity}
        />
      </>
    )
  },
})
