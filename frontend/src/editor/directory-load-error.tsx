import { createComponent, Shade } from "@furystack/shades";
import { Alert, Button, cssVariableTheme } from "@furystack/shades-common-components";

import { CosFilesError, describeCosError } from "../ide/files/cos-files";
import { DeviceAccess } from "./device-access";

export interface DirectoryLoadErrorProps {
  error: unknown;
  onRetry: () => void;
}

/** Cache error for a directory listing. A 401 also asks for credentials. */
export const DirectoryLoadError = Shade<DirectoryLoadErrorProps>({
  customElementName: "cos-directory-load-error",
  css: {
    display: "block",
    padding: cssVariableTheme.spacing.sm,
    "& shade-button": {
      margin: `${cssVariableTheme.spacing.sm} 0 0`,
    },
  },
  render: ({ props, injector, useDisposable }) => {
    const unauthorized = props.error instanceof CosFilesError && props.error.status === 401;
    useDisposable("unauthorized", () => {
      if (unauthorized) {
        queueMicrotask(() => injector.get(DeviceAccess).markUnauthorized());
      }
      return { [Symbol.dispose]() {} };
    });

    return (
      <div>
        <Alert severity="error" variant="outlined" title={describeCosError(props.error)} />
        <Button variant="outlined" size="small" type="button" onclick={props.onRetry}>
          Retry
        </Button>
      </div>
    );
  },
});
