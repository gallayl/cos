import { createComponent, Shade } from "@furystack/shades";
import { Button, ButtonGroup, CacheView, Typography, cssVariableTheme } from "@furystack/shades-common-components";

import { DeviceAccess } from "./device-access";
import { DeviceSignIn } from "./device-sign-in";
import { DirectoryList } from "./directory-list";
import { DirectoryLoadError } from "./directory-load-error";
import { DirectorySkeleton } from "./directory-skeleton";
import { FileCache } from "./file-cache";

const roots = ["/flash", "/sdcard"] as const;

export interface FileBrowserProps {
  directory: string;
  activePath: string;
  onDirectoryChange: (path: string) => void;
  onOpenFile: (path: string) => void;
}

/** Device filesystem browser. Listings come from {@link FileCache} through {@link CacheView}. */
export const FileBrowser = Shade<FileBrowserProps>({
  customElementName: "cos-file-browser",
  css: {
    display: "flex",
    flexDirection: "column",
    width: "16rem",
    minWidth: "11rem",
    height: "100%",
    minHeight: "0",
    background: cssVariableTheme.background.paper,
    borderRight: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
    "& .roots": {
      padding: cssVariableTheme.spacing.sm,
      borderBottom: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
    },
    "& .roots shade-button-group": {
      display: "flex",
      width: "100%",
    },
    "& .roots shade-button": {
      flex: "1",
    },
    "& .path": {
      padding: `${cssVariableTheme.spacing.sm} ${cssVariableTheme.spacing.md} 0`,
    },
    "& shade-cache-view": {
      flex: "1",
      minHeight: "0",
      overflow: "auto",
    },
  },
  render: ({ props, injector, useObservable }) => {
    const access = injector.get(DeviceAccess);
    const cache = injector.get(FileCache);
    const [needsAuth] = useObservable("needs-auth", access.needsAuth);
    const [authorization] = useObservable("authorization", access.authorization);

    const openRoot = (path: string) => {
      if (path === props.directory) {
        cache.refreshDirectory(path, access.authorization.getValue());
        return;
      }
      props.onDirectoryChange(path);
    };

    const openEntry = (path: string, isDirectory: boolean) => {
      if (isDirectory) {
        props.onDirectoryChange(path);
        return;
      }
      props.onOpenFile(path);
    };

    return (
      <>
        {needsAuth ? <DeviceSignIn /> : null}
        <div className="roots">
          <ButtonGroup variant="outlined">
            {roots.map((root) => (
              <Button type="button" size="small" onclick={() => openRoot(root)}>
                {root.slice(1)}
              </Button>
            ))}
          </ButtonGroup>
        </div>
        <Typography className="path" variant="caption" color="textSecondary">
          {props.directory}
        </Typography>
        <CacheView
          cache={cache.directories}
          args={[props.directory, authorization]}
          loader={<DirectorySkeleton />}
          content={DirectoryList}
          contentProps={{
            directory: props.directory,
            activePath: props.activePath,
            onOpen: openEntry,
          }}
          error={(error, retry) => <DirectoryLoadError error={error} onRetry={retry} />}
        />
      </>
    );
  },
});
