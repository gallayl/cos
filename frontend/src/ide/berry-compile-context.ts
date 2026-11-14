import { defineService } from '@furystack/inject'
import { ObservableValue } from '@furystack/utils'

import type { BerryCompileMetadata } from './ast-to-berry'

/**
 * Shares the latest Berry compile debug metadata across frontend features
 * (editor, console) so runtime/device output can be mapped back to TypeScript.
 */
export interface BerryCompileContextService extends Disposable {
  readonly metadata: ObservableValue<BerryCompileMetadata | undefined>
  setMetadata(metadata: BerryCompileMetadata | undefined): void
}

export const BerryCompileContext = defineService({
  name: 'cos/BerryCompileContext',
  lifetime: 'singleton',
  factory: ({ onDispose }) => {
    /* eslint-disable furystack/prefer-using-wrapper -- ObservableValue is created here but owned by the returned service; disposal is deferred to service lifetime, so a same-scope using() wrapper cannot express this ownership transfer. */
    const metadata = new ObservableValue<BerryCompileMetadata | undefined>(undefined)
    const service: BerryCompileContextService = {
      metadata,
      setMetadata(next) {
        metadata.setValue(next)
      },
      [Symbol.dispose]() {
        metadata[Symbol.dispose]()
      },
    }
    onDispose(() => service[Symbol.dispose]())
    return service
    /* eslint-enable furystack/prefer-using-wrapper */
  },
})
