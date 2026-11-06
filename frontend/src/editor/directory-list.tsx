import type { CacheWithValue } from "@furystack/cache";
import { createComponent, Shade } from "@furystack/shades";
import { cssVariableTheme } from "@furystack/shades-common-components";

import type { CosDirEntry } from "../ide/files/cos-files";
import { joinPath, parentPath } from "../ide/files/paths";
import { FileEntry } from "./file-entry";

export interface DirectoryListProps {
  data: CacheWithValue<CosDirEntry[]>;
  directory: string;
  activePath: string;
  onOpen: (path: string, isDirectory: boolean) => void;
}

/** Directory rows for a loaded cache entry, including the parent link. */
export const DirectoryList = Shade<DirectoryListProps>({
  customElementName: "cos-directory-list",
  css: {
    display: "block",
    "& ul": {
      margin: "0",
      padding: cssVariableTheme.spacing.xs,
      listStyle: "none",
    },
    "& li": {
      margin: "0",
    },
    "& cos-file-entry": {
      margin: "0",
    },
  },
  render: ({ props }) => {
    const parent = parentPath(props.directory);
    return (
      <ul>
        {parent ? (
          <li>
            <FileEntry name=".." path={parent} isDirectory active={false} onOpen={props.onOpen} />
          </li>
        ) : null}
        {props.data.value.map((entry) => {
          const path = joinPath(props.directory, entry.name);
          return (
            <li>
              <FileEntry
                name={entry.name}
                path={path}
                isDirectory={entry.is_dir}
                active={path === props.activePath}
                onOpen={props.onOpen}
              />
            </li>
          );
        })}
      </ul>
    );
  },
});
