import { createComponent, Shade } from '@furystack/shades'
import { Alert, Button, Input, cssVariableTheme } from '@furystack/shades-common-components'

import { DeviceAccess } from './device-access'

/** Username and password for the device filesystem API. */
export const DeviceSignIn = Shade({
  customElementName: 'cos-device-sign-in',
  css: {
    display: 'block',
    '& form': {
      display: 'flex',
      flexDirection: 'column',
      gap: cssVariableTheme.spacing.sm,
      padding: cssVariableTheme.spacing.sm,
      borderBottom: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
    },
    '& shade-input': {
      marginBottom: '0',
    },
    '& shade-button': {
      margin: '0',
    },
  },
  render: ({ injector, useState }) => {
    const [username, setUsername] = useState('username', '')
    const [password, setPassword] = useState('password', '')
    const [message, setMessage] = useState('message', '')
    const access = injector.get(DeviceAccess)

    const submit = () => {
      const failure = access.signIn(username, password)
      setMessage(failure ?? '')
    }

    return (
      <form
        onsubmit={(event) => {
          event.preventDefault()
          submit()
        }}
      >
        <Input
          type="text"
          labelTitle="Username"
          placeholder="Username"
          autocomplete="username"
          variant="outlined"
          size="small"
          value={username}
          oninput={(event) => setUsername((event.target as HTMLInputElement).value)}
        />
        <Input
          type="password"
          labelTitle="Password"
          placeholder="Password"
          autocomplete="current-password"
          variant="outlined"
          size="small"
          value={password}
          oninput={(event) => setPassword((event.target as HTMLInputElement).value)}
        />
        {message ? <Alert severity="error" variant="outlined" title={message} /> : null}
        <Button variant="contained" color="primary" size="small" type="button" onclick={submit}>
          Sign in
        </Button>
      </form>
    )
  },
})
