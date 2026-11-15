import { createComponent, Shade } from '@furystack/shades'
import {
  ContextMenu,
  ContextMenuManager,
  Icon,
  NotyService,
  Skeleton,
  Tree,
  TreeService,
  cssVariableTheme,
  icons,
} from '@furystack/shades-common-components'

import {
  CosFilesError,
  createDirectory,
  deleteEntry,
  describeCosError,
  renameEntry,
  uploadFile,
  writeTextFile,
  type CosDirEntry,
} from '../ide/files/cos-files'
import { isTypeScriptPath, joinPath } from '../ide/files/paths'
import {
  moveWorkspaceTypeScriptTree,
  registerWorkspaceTypeScriptFile,
  unregisterWorkspaceTypeScriptTree,
} from '../ide/typescript-workspace'
import { DeviceAccess } from './device-access'
import { DirectoryLoadError } from './directory-load-error'
import { collectDrop, entriesFromDataTransfer } from './file-drop'
import { FileCache } from './file-cache'
import { FileNameField } from './file-name-field'
import {
  FileTreeStore,
  childPath,
  commandDirectory,
  directoriesToCreate,
  menuActions,
  nameSelection,
  shouldReleaseTreeFocus,
  type FileMenuAction,
  type FileNode,
} from './file-tree'

export interface FileTreeViewProps {
  activePath: string
  onOpenFile: (path: string) => void
  onMoved: (from: string, to: string) => void
  onActivity: (message: string) => void
}

const actionLabel: Record<FileMenuAction, string> = {
  'new-file': 'New File',
  'new-folder': 'New Folder',
  rename: 'Rename',
  delete: 'Delete',
}

/** Expandable device filesystem. Listings load when a folder is opened. */
export const FileTreeView = Shade<FileTreeViewProps>({
  customElementName: 'cos-file-tree-view',
  css: {
    display: 'flex',
    flexDirection: 'column',
    minHeight: '0',
    height: '100%',
    '& .tree': {
      flex: '1',
      minHeight: '0',
      display: 'flex',
      flexDirection: 'column',
    },
    '& shade-tree': {
      flex: '1',
      minHeight: '0',
    },
    '& .name': {
      display: 'block',
      overflow: 'hidden',
      textOverflow: 'ellipsis',
      whiteSpace: 'nowrap',
      fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Consolas, monospace',
      fontSize: cssVariableTheme.typography.fontSize.sm,
    },
    '& .name[data-active]': {
      fontWeight: cssVariableTheme.typography.fontWeight.bold,
    },
    '& .name[data-drop]': {
      outline: `1px solid ${cssVariableTheme.palette.primary.main}`,
      borderRadius: cssVariableTheme.shape.borderRadius.sm,
    },
    '& .pending': {
      display: 'block',
      height: '1.25rem',
    },
    '&[data-drop]': {
      background: cssVariableTheme.action.hoverBackground,
    },
  },
  render: ({ props, injector, useDisposable, useObservable, useState, useHostProps }) => {
    const access = injector.get(DeviceAccess)
    const cache = injector.get(FileCache)
    const notyService = injector.get(NotyService)
    const store = useDisposable('store', () => new FileTreeStore())
    const treeService = useDisposable('tree', () => {
      const service = new TreeService<FileNode>({ getChildren: store.getChildren })
      service.rootItems.setValue(store.roots)
      service.expand(store.roots[0])
      service.updateFlattenedNodes()
      return service
    })
    const menu = useDisposable('menu', () => new ContextMenuManager<FileMenuAction>())
    const menuNode = useDisposable('menu-node', () => ({
      current: undefined as FileNode | undefined,
      [Symbol.dispose]() {},
    }))
    const resync = useDisposable('resync', () => ({
      current: () => {},
      [Symbol.dispose]() {},
    }))
    const [revision] = useObservable('revision', store.revision)
    const [dropPath, setDropPath] = useState('drop-path', '')
    void revision

    const notify = (type: 'success' | 'error' | 'info' | 'warning', title: string, body?: string) => {
      notyService.emit('onNotyAdded', { type, title, body: body ?? '' })
    }

    const fail = (error: unknown, title = 'Operation failed') => {
      if (error instanceof CosFilesError && error.status === 401) {
        access.markUnauthorized()
      }
      const message = describeCosError(error)
      props.onActivity(message)
      notify('error', title, message)
    }

    useDisposable('release-tree-focus', () => {
      const listener = (event: Event) => {
        const path = event.composedPath().flatMap((entry) => {
          if (!(entry instanceof HTMLElement)) {
            return []
          }
          return [{ tagName: entry.tagName, isContentEditable: entry.isContentEditable }]
        })
        if (!shouldReleaseTreeFocus(path)) {
          return
        }
        if (treeService.focusedItem.getValue() !== undefined) {
          treeService.focusedItem.setValue(undefined)
        }
        if (treeService.hasFocus.getValue()) {
          treeService.hasFocus.setValue(false)
        }
      }
      window.addEventListener('pointerdown', listener, true)
      window.addEventListener('focusin', listener, true)
      return {
        [Symbol.dispose]() {
          window.removeEventListener('pointerdown', listener, true)
          window.removeEventListener('focusin', listener, true)
        },
      }
    })

    useDisposable('listings', () => {
      let authorization = access.authorization.getValue()
      const watchers = new Map<string, { [Symbol.dispose](): void }>()

      const apply = (path: string, result: { status: string; value?: CosDirEntry[]; error?: unknown }) => {
        if (result.status === 'failed' && result.value === undefined) {
          if (result.error instanceof CosFilesError && result.error.status === 401) {
            access.markUnauthorized()
          }
          store.setError(path, describeCosError(result.error))
          treeService.updateFlattenedNodes()
          return
        }
        const showsEntries =
          result.value !== undefined &&
          (result.status === 'loaded' || result.status === 'obsolete' || result.status === 'loading')
        if (showsEntries && result.value) {
          store.setListing(path, result.value)
          treeService.updateFlattenedNodes()
        }
      }

      const watch = (path: string) => {
        const observable = cache.directories.getObservable(path, authorization)
        apply(path, observable.getValue())
        return observable.subscribe(() => apply(path, observable.getValue()))
      }

      const sync = () => {
        const wanted = new Set<string>()
        for (const node of treeService.expandedNodes.getValue()) {
          if (node.kind === 'directory') {
            wanted.add(node.path)
          }
        }
        for (const [path, watcher] of watchers) {
          if (!wanted.has(path)) {
            watcher[Symbol.dispose]()
            watchers.delete(path)
          }
        }
        for (const path of wanted) {
          if (!watchers.has(path)) {
            watchers.set(path, watch(path))
          }
        }
      }

      resync.current = () => {
        for (const watcher of watchers.values()) {
          watcher[Symbol.dispose]()
        }
        watchers.clear()
        sync()
      }

      const expanded = treeService.expandedNodes.subscribe(() => sync())
      const authorizationChanges = access.authorization.subscribe((next) => {
        authorization = next
        store.clearListings()
        treeService.updateFlattenedNodes()
        resync.current()
      })
      sync()

      return {
        [Symbol.dispose]() {
          resync.current = () => {}
          expanded[Symbol.dispose]()
          authorizationChanges[Symbol.dispose]()
          for (const watcher of watchers.values()) {
            watcher[Symbol.dispose]()
          }
        },
      }
    })

    const refreshTree = () => {
      store.notify()
      treeService.updateFlattenedNodes()
    }

    const startCreate = async (directory: string, draftKind: 'file' | 'directory') => {
      const folder = store.findByPath(directory)
      if (folder?.kind === 'directory') {
        treeService.expand(folder)
      }
      try {
        const entries = await cache.directories.get(directory, access.authorization.getValue())
        store.setListing(directory, entries)
        store.insertDraft(directory, draftKind)
        treeService.updateFlattenedNodes()
      } catch (error) {
        fail(error)
      }
    }

    const commitName = async (node: FileNode, name: string) => {
      const previousName = node.name
      node.editing = false
      if (node.kind === 'draft') {
        node.name = name
      }
      refreshTree()
      const authorization = access.authorization.getValue()
      const options = { authorization }
      try {
        if (node.kind === 'draft') {
          const path = joinPath(node.parentPath, name)
          if (node.draftKind === 'directory') {
            await createDirectory(path, options)
          } else {
            await writeTextFile(path, '', options)
            cache.rememberFile(path, authorization, '')
            if (isTypeScriptPath(path)) {
              registerWorkspaceTypeScriptFile(path, '')
            }
          }
          store.removeDraft(node)
          cache.refreshDirectory(node.parentPath, authorization)
          treeService.updateFlattenedNodes()
          props.onActivity(`Created ${path}`)
          notify('success', node.draftKind === 'directory' ? 'Folder created' : 'File created', path)
          if (node.draftKind !== 'directory') {
            props.onOpenFile(path)
          }
          return
        }
        if (name === previousName) {
          return
        }
        const to = joinPath(node.parentPath, name)
        const from = node.path
        await renameEntry(from, to, options)
        store.applyRename(node, name)
        cache.invalidatePathTree(from, authorization)
        moveWorkspaceTypeScriptTree(from, to)
        cache.refreshDirectory(node.parentPath, authorization)
        resync.current()
        treeService.updateFlattenedNodes()
        props.onMoved(from, to)
        props.onActivity(`Renamed to ${to}`)
        notify('success', 'Renamed', `${from} → ${to}`)
      } catch (error) {
        if (node.kind === 'draft') {
          node.name = name
          node.editing = true
          store.notify()
        } else {
          node.name = previousName
          store.beginRename(node)
        }
        treeService.updateFlattenedNodes()
        fail(error, node.kind === 'draft' ? 'Create failed' : 'Rename failed')
      }
    }

    const removeNode = async (node: FileNode) => {
      const label = node.kind === 'directory' ? `Delete folder ${node.name}? It must be empty.` : `Delete ${node.name}?`
      if (!window.confirm(label)) {
        return
      }
      const authorization = access.authorization.getValue()
      try {
        await deleteEntry(node.path, { authorization })
        cache.invalidatePathTree(node.path, authorization)
        unregisterWorkspaceTypeScriptTree(node.path)
        treeService.collapse(node)
        store.removeLocal(node)
        if (node.parentPath) {
          cache.refreshDirectory(node.parentPath, authorization)
        }
        treeService.updateFlattenedNodes()
        props.onActivity(`Deleted ${node.path}`)
        notify('success', node.kind === 'directory' ? 'Folder deleted' : 'File deleted', node.path)
      } catch (error) {
        fail(error, 'Delete failed')
      }
    }

    const uploadTo = async (directory: string, data: DataTransfer) => {
      const folder = store.findByPath(directory)
      if (folder?.kind === 'directory') {
        treeService.expand(folder)
      }
      let collected
      try {
        collected = await collectDrop(entriesFromDataTransfer(data))
      } catch (error) {
        fail(error, 'Upload failed')
        return
      }
      const relativeDirectories = directoriesToCreate(
        collected.directories,
        collected.files.map((file) => file.relativePath),
      )
      const files = collected.files.flatMap((file) => {
        const path = childPath(directory, file.relativePath)
        return path ? [{ path, blob: file.blob }] : []
      })
      if (relativeDirectories.length === 0 && files.length === 0) {
        props.onActivity('Nothing to upload')
        notify('info', 'Nothing to upload', directory)
        return
      }
      const authorization = access.authorization.getValue()
      const options = { authorization }
      try {
        for (const relative of relativeDirectories) {
          const path = childPath(directory, relative)
          if (!path) {
            continue
          }
          try {
            await createDirectory(path, options)
          } catch (error) {
            if (!(error instanceof CosFilesError) || error.message !== 'Already exists') {
              throw error
            }
          }
        }
        for (let index = 0; index < files.length; index += 1) {
          const file = files[index]
          if (!file) {
            continue
          }
          props.onActivity(`Uploading ${index + 1}/${files.length} ${file.path}`)
          await uploadFile(file.path, file.blob, options)
          cache.invalidateFile(file.path, authorization)
          if (isTypeScriptPath(file.path) && isProbablyTextBlob(file.blob)) {
            try {
              const text = await file.blob.text()
              cache.rememberFile(file.path, authorization, text)
              registerWorkspaceTypeScriptFile(file.path, text)
            } catch {
              // Best-effort: binary or unreadable uploads still succeed on device.
            }
          }
        }
        cache.refreshDirectory(directory, authorization)
        const message =
          files.length > 0 ? `Uploaded ${files.length} to ${directory}` : `Created folders in ${directory}`
        props.onActivity(message)
        notify('success', files.length > 0 ? 'Upload complete' : 'Folders created', message)
      } catch (error) {
        cache.refreshDirectory(directory, authorization)
        fail(error, 'Upload failed')
      }
    }

    const runAction = (action: FileMenuAction) => {
      const node = menuNode.current
      const directory = commandDirectory(node)
      if (action === 'new-file' || action === 'new-folder') {
        void startCreate(directory, action === 'new-folder' ? 'directory' : 'file')
        return
      }
      if (!node || node.kind === 'pending' || node.kind === 'error' || node.kind === 'draft') {
        return
      }
      if (action === 'rename') {
        store.beginRename(node)
        treeService.updateFlattenedNodes()
        return
      }
      void removeNode(node)
    }

    const nodeAt = (event: Event): FileNode | undefined => {
      const row = event.composedPath().find((entry): entry is HTMLElement => {
        return entry instanceof HTMLElement && entry.tagName === 'SHADE-TREE-ITEM'
      })
      if (!row?.parentElement) {
        return undefined
      }
      const rows = [...row.parentElement.children].filter((entry) => entry.tagName === 'SHADE-TREE-ITEM')
      return treeService.flattenedNodes.getValue()[rows.indexOf(row)]?.item
    }

    const openMenu = (event: MouseEvent) => {
      event.preventDefault()
      const node = nodeAt(event)
      menuNode.current = node
      treeService.hasFocus.setValue(false)
      const actions = menuActions(node)
      menu.open({
        position: { x: event.clientX, y: event.clientY },
        items: actions.flatMap((action) => [
          ...(action === 'rename' ? [{ type: 'separator' as const }] : []),
          {
            type: 'item' as const,
            data: action,
            label: actionLabel[action],
            icon: (
              <Icon
                icon={
                  action === 'new-folder'
                    ? icons.folder
                    : action === 'rename'
                      ? icons.edit
                      : action === 'delete'
                        ? icons.trash
                        : icons.file
                }
                size="small"
              />
            ),
          },
        ]),
      })
    }

    const isProbablyTextBlob = (blob: Blob): boolean => {
      const type = blob.type.toLowerCase()
      return (
        type === '' ||
        type.startsWith('text/') ||
        type.includes('json') ||
        type.includes('javascript') ||
        type.includes('typescript')
      )
    }

    useHostProps(dropPath ? { 'data-drop': '' } : {})

    return (
      <div
        className="tree"
        ondragover={(event) => {
          event.preventDefault()
          if (event.dataTransfer) {
            event.dataTransfer.dropEffect = 'copy'
          }
          setDropPath(commandDirectory(nodeAt(event)))
        }}
        ondragleave={(event) => {
          const next = event.relatedTarget
          const host = event.currentTarget as HTMLElement
          if (!(next instanceof Node) || !host.contains(next)) {
            setDropPath('')
          }
        }}
        ondrop={(event) => {
          event.preventDefault()
          const data = event.dataTransfer
          const directory = commandDirectory(nodeAt(event))
          setDropPath('')
          if (data) {
            void uploadTo(directory, data)
          }
        }}
        onclick={(event) => {
          const { target } = event
          if (target instanceof Element && target.closest('cos-file-name-field, shade-button')) {
            return
          }
          const node = nodeAt(event)
          if (node?.kind === 'directory') {
            treeService.toggleExpanded(node)
            return
          }
          if (node?.kind === 'file') {
            props.onOpenFile(node.path)
          }
        }}
        oncontextmenu={(event) => openMenu(event)}
      >
        <Tree
          rootItems={store.roots}
          treeService={treeService}
          onItemActivate={(node) => {
            if (node.kind === 'file') {
              props.onOpenFile(node.path)
            }
          }}
          renderIcon={(node, expanded) => {
            const directory = node.kind === 'directory' || (node.kind === 'draft' && node.draftKind === 'directory')
            if (directory) {
              return (
                <Icon
                  icon={expanded && node.kind === 'directory' ? icons.folderOpen : icons.folder}
                  size="small"
                  color="info"
                />
              )
            }
            if (node.kind === 'file' || node.kind === 'draft') {
              return <Icon icon={icons.file} size="small" />
            }
            return <span />
          }}
          renderItem={(node) => {
            if (node.kind === 'pending') {
              return (
                <div className="pending">
                  <Skeleton delay={0} />
                </div>
              )
            }
            if (node.kind === 'error') {
              return (
                <DirectoryLoadError
                  error={node.message ?? 'Request failed'}
                  onRetry={() => cache.refreshDirectory(node.parentPath, access.authorization.getValue())}
                />
              )
            }
            if (node.editing) {
              const directory = node.kind === 'directory' || node.draftKind === 'directory'
              const selection = nameSelection(node.name, directory)
              return (
                <FileNameField
                  initialValue={node.name}
                  placeholder={directory ? 'folder name' : 'file name'}
                  selectFrom={selection.start}
                  selectTo={selection.end}
                  validate={(name) => {
                    if (store.hasChildName(node.parentPath, name, node.kind === 'draft' ? undefined : node)) {
                      return 'That name already exists'
                    }
                    return null
                  }}
                  onInvalid={props.onActivity}
                  onCancel={() => {
                    store.cancelEdit()
                    treeService.updateFlattenedNodes()
                  }}
                  onCommit={(name) => void commitName(node, name)}
                />
              )
            }
            return (
              <span
                className="name"
                title={node.path}
                {...(node.path === props.activePath ? { 'data-active': '' } : {})}
                {...(node.kind === 'directory' && node.path === dropPath ? { 'data-drop': '' } : {})}
              >
                {node.name}
              </span>
            )
          }}
        />
        <ContextMenu manager={menu} onItemSelect={(action) => runAction(action)} />
      </div>
    )
  },
})
