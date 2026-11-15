import { describe, expect, it } from 'vitest'

import {
  FileTreeStore,
  childPath,
  commandDirectory,
  directoriesToCreate,
  entryNameError,
  menuActions,
  movedPath,
  nameSelection,
  shouldReleaseTreeFocus,
  type FileNode,
} from './file-tree'

describe('file tree paths', () => {
  it('rejects unsafe names and keeps a file extension out of the selection', () => {
    expect(entryNameError('app.ts')).toBeNull()
    expect(entryNameError('..')).toBe('Use a name without slashes')
    expect(entryNameError('a/b')).toBe('Use a name without slashes')
    expect(entryNameError(' app')).toBe('Use a name without slashes')
    expect(nameSelection('app.ts', false)).toEqual({ start: 0, end: 3 })
    expect(nameSelection('.gitignore', false)).toEqual({ start: 0, end: 10 })
    expect(nameSelection('apps', true)).toEqual({ start: 0, end: 4 })
    expect(shouldReleaseTreeFocus([{ tagName: 'DIV' }, { tagName: 'MONACO-EDITOR' }])).toBe(true)
    expect(shouldReleaseTreeFocus([{ tagName: 'INPUT' }, { tagName: 'COS-FILE-TREE-VIEW' }])).toBe(true)
    expect(
      shouldReleaseTreeFocus([{ tagName: 'DIV', isContentEditable: true }, { tagName: 'COS-FILE-TREE-VIEW' }]),
    ).toBe(true)
    expect(shouldReleaseTreeFocus([{ tagName: 'SHADE-TREE-ITEM' }, { tagName: 'COS-FILE-TREE-VIEW' }])).toBe(false)
  })

  it('remaps renamed paths and joins dropped relative paths', () => {
    expect(movedPath('/flash/apps/app.ts', '/flash/apps', '/flash/widgets')).toBe('/flash/widgets/app.ts')
    expect(movedPath('/flash/apps', '/flash/apps', '/flash/widgets')).toBe('/flash/widgets')
    expect(movedPath('/flash/other.ts', '/flash/apps', '/flash/widgets')).toBeNull()
    expect(childPath('/flash', 'apps/app.ts')).toBe('/flash/apps/app.ts')
    expect(childPath('/flash', 'apps/../secret')).toBeNull()
    expect(directoriesToCreate(['empty'], ['apps/demo/app.ts'])).toEqual(['apps', 'empty', 'apps/demo'])
  })

  it('offers create on roots and rename on files', () => {
    const root = directory('/flash', '')
    const folder = directory('/flash/apps', '/flash')
    const file = fileNode('/flash/apps/app.ts', '/flash/apps')
    expect(menuActions(root)).toEqual(['new-file', 'new-folder'])
    expect(menuActions(folder)).toEqual(['new-file', 'new-folder', 'rename', 'delete'])
    expect(menuActions(file)).toEqual(['rename', 'delete'])
    expect(commandDirectory(file)).toBe('/flash/apps')
    expect(commandDirectory(folder)).toBe('/flash/apps')
    expect(commandDirectory(undefined)).toBe('/flash')
  })
})

describe('file tree store', () => {
  it('reuses entries when a directory is listed again', () => {
    const store = new FileTreeStore()
    expect(store.getChildren(store.roots[0])[0]?.kind).toBe('pending')

    store.setListing('/flash', [
      { name: 'apps', is_dir: true, size: 0, mtime: 0, ctime: 0 },
      { name: 'app.ts', is_dir: false, size: 1, mtime: 0, ctime: 0 },
    ])
    const first = store.getChildren(store.roots[0])
    store.setListing('/flash', [
      { name: 'apps', is_dir: true, size: 0, mtime: 0, ctime: 0 },
      { name: 'app.ts', is_dir: false, size: 1, mtime: 0, ctime: 0 },
    ])
    const second = store.getChildren(store.roots[0])
    expect(second[0]).toBe(first[0])
    expect(second[1]).toBe(first[1])
    expect(store.getChildren(first[0])).toEqual([
      expect.objectContaining({ kind: 'pending', parentPath: '/flash/apps' }),
    ])
  })

  it('renames a folder and its loaded children', () => {
    const store = new FileTreeStore()
    store.setListing('/flash', [{ name: 'apps', is_dir: true, size: 0, mtime: 0, ctime: 0 }])
    const folder = store.getChildren(store.roots[0])[0]
    if (!folder) {
      throw new Error('missing folder')
    }
    store.setListing('/flash/apps', [{ name: 'app.ts', is_dir: false, size: 1, mtime: 0, ctime: 0 }])
    const file = store.getChildren(folder)[0]

    expect(store.applyRename(folder, 'widgets')).toEqual({ from: '/flash/apps', to: '/flash/widgets' })
    expect(folder.path).toBe('/flash/widgets')
    expect(file?.path).toBe('/flash/widgets/app.ts')
    expect(file?.parentPath).toBe('/flash/widgets')
    expect(store.findByPath('/flash/widgets/app.ts')).toBe(file)
    expect(store.hasChildName('/flash', 'widgets')).toBe(true)
  })

  it('keeps a draft across a refresh and drops it when editing is cancelled', () => {
    const store = new FileTreeStore()
    store.setListing('/flash', [{ name: 'app.ts', is_dir: false, size: 1, mtime: 0, ctime: 0 }])
    const draft = store.insertDraft('/flash', 'file')
    store.setListing('/flash', [{ name: 'app.ts', is_dir: false, size: 1, mtime: 0, ctime: 0 }])
    expect(store.getChildren(store.roots[0])[0]).toBe(draft)

    store.cancelEdit()
    expect(store.getChildren(store.roots[0]).some((node) => node.kind === 'draft')).toBe(false)
  })

  it('shows an error only before any entries arrive', () => {
    const store = new FileTreeStore()
    store.setError('/sdcard', 'Not found')
    expect(store.getChildren(store.roots[1])[0]).toMatchObject({ kind: 'error', message: 'Not found' })

    store.setListing('/sdcard', [{ name: 'notes.txt', is_dir: false, size: 1, mtime: 0, ctime: 0 }])
    store.setError('/sdcard', 'Cannot reach the device')
    expect(store.getChildren(store.roots[1])[0]?.kind).toBe('file')
  })
})

function directory(path: string, parentPath: string): FileNode {
  return { kind: 'directory', name: path.slice(parentPath.length + 1), path, parentPath, editing: false }
}

function fileNode(path: string, parentPath: string): FileNode {
  return { kind: 'file', name: path.slice(parentPath.length + 1), path, parentPath, editing: false }
}
