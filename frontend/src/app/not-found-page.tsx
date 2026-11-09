import { createComponent, Shade, nestedNavigate } from '@furystack/shades'
import { Button, PageContainer, Result, cssVariableTheme } from '@furystack/shades-common-components'

/** Shown when the current URL does not match a route. */
export const NotFoundPage = Shade({
  customElementName: 'cos-not-found-page',
  css: {
    display: 'flex',
    width: '100%',
    height: '100%',
    minHeight: '0',
    alignItems: 'center',
    justifyContent: 'center',
    background: cssVariableTheme.background.default,
    color: cssVariableTheme.text.primary,
  },
  render: ({ injector }) => (
    <PageContainer maxWidth="32rem" centered fullHeight={false}>
      <Result status="404" title="Page not found" subtitle="That address is not part of COS.">
        <Button type="button" variant="contained" onclick={() => nestedNavigate(injector, { path: '/' })}>
          Back to home
        </Button>
      </Result>
    </PageContainer>
  ),
})
