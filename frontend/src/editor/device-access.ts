import { defineService } from '@furystack/inject'
import { ObservableValue } from '@furystack/utils'

import { authorizationFromCredentials } from './editor-document'

/** `sessionStorage` key for the shared HTTP Basic header. */
export const deviceAuthorizationKey = 'cos.basic-authorization'

/** Credentials for the device filesystem API and the console websocket. */
export interface DeviceAccessService {
  /** Authorization header value, or an empty string when the user has not signed in. */
  readonly authorization: ObservableValue<string>
  /** True after a device call returns 401. */
  readonly needsAuth: ObservableValue<boolean>
  /**
   * Store HTTP Basic credentials.
   * @returns An error message, or null when the header was stored.
   */
  signIn(username: string, password: string): string | null
  /** Show the sign-in form after a device call rejects the current credentials. */
  markUnauthorized(): void
}

type AuthorizationStorage = Pick<Storage, 'getItem' | 'setItem' | 'removeItem'>

/** Browser storage for the Basic header. Missing storage keeps credentials in memory only. */
export function browserAuthorizationStorage(): AuthorizationStorage | null {
  try {
    return globalThis.sessionStorage ?? null
  } catch {
    return null
  }
}

/**
 * Shared device credentials.
 * The header is kept in `sessionStorage` so a refresh uses the same Basic auth as the editor.
 */
export function createDeviceAccess(
  storage: AuthorizationStorage | null = browserAuthorizationStorage(),
): DeviceAccessService & Disposable {
  const authorization = new ObservableValue(storage?.getItem(deviceAuthorizationKey) ?? '')
  const needsAuth = new ObservableValue(false)
  const service: DeviceAccessService & Disposable = {
    authorization,
    needsAuth,
    signIn(username, password) {
      const result = authorizationFromCredentials(username, password)
      if (!result.ok) {
        return result.message
      }
      storage?.setItem(deviceAuthorizationKey, result.authorization)
      authorization.setValue(result.authorization)
      needsAuth.setValue(false)
      return null
    },
    markUnauthorized() {
      needsAuth.setValue(true)
    },
    [Symbol.dispose]() {
      /* eslint-disable furystack/prefer-using-wrapper -- composite disposal of owned service state */
      authorization[Symbol.dispose]()
      needsAuth[Symbol.dispose]()
      /* eslint-enable furystack/prefer-using-wrapper */
    },
  }
  return service
}

/** Shared device credentials for the file browser, editor, and console. */
export const DeviceAccess = defineService({
  name: 'cos/DeviceAccess',
  lifetime: 'singleton',
  factory: ({ onDispose }) => {
    const service = createDeviceAccess()
    onDispose(() => service[Symbol.dispose]())
    return service
  },
})
