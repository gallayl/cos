import { defineService } from "@furystack/inject";
import { ObservableValue } from "@furystack/utils";

import { authorizationFromCredentials } from "./editor-document";

/** Credentials for the device filesystem API. */
export interface DeviceAccessService {
  /** Authorization header value, or an empty string when the user has not signed in. */
  readonly authorization: ObservableValue<string>;
  /** True after a device call returns 401. */
  readonly needsAuth: ObservableValue<boolean>;
  /**
   * Store HTTP Basic credentials.
   * @returns An error message, or null when the header was stored.
   */
  signIn(username: string, password: string): string | null;
  /** Show the sign-in form after a device call rejects the current credentials. */
  markUnauthorized(): void;
}

/** Shared device credentials for the file browser and editor actions. */
export const DeviceAccess = defineService({
  name: "cos/DeviceAccess",
  lifetime: "singleton",
  factory: ({ onDispose }) => {
    const authorization = new ObservableValue("");
    const needsAuth = new ObservableValue(false);
    onDispose(() => {
      authorization[Symbol.dispose]();
      needsAuth[Symbol.dispose]();
    });

    const service: DeviceAccessService = {
      authorization,
      needsAuth,
      signIn(username, password) {
        const result = authorizationFromCredentials(username, password);
        if (!result.ok) {
          return result.message;
        }
        authorization.setValue(result.authorization);
        needsAuth.setValue(false);
        return null;
      },
      markUnauthorized() {
        needsAuth.setValue(true);
      },
    };
    return service;
  },
});
