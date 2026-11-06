import { createComponent, NestedRouteLink, Shade } from "@furystack/shades";
import {
  Alert,
  Button,
  Input,
  SegmentedControl,
  Typography,
  cssVariableTheme,
} from "@furystack/shades-common-components";

import { DeviceAccess } from "../editor/device-access";
import { DeviceSignIn } from "../editor/device-sign-in";
import { openConsoleLink, type ConsoleLink, type ConsoleLinkPhase } from "./console-link";
import {
  consoleMessageLimit,
  createConsoleTranscript,
  isConsoleSeverity,
  lineVisible,
  normalizeConsoleMessage,
  pushConsoleChunk,
  severityOf,
  stripAnsi,
  type ConsoleSeverity,
  type ConsoleTranscript,
} from "./console-lines";

const levelOptions = [
  { value: "error", label: "Error" },
  { value: "warn", label: "Warn" },
  { value: "info", label: "Info" },
];

const phaseText: Record<ConsoleLinkPhase | "signed-out", string> = {
  "signed-out": "Sign in to use the console",
  connecting: "Connecting",
  open: "Connected",
  unauthorized: "The device rejected these credentials",
  unreachable: "Cannot reach the device",
  closed: "Disconnected",
};

/** Device console: filtered logs and one command at a time. */
export const ConsolePage = Shade({
  customElementName: "cos-console-page",
  css: {
    display: "flex",
    flexDirection: "column",
    width: "100%",
    height: "100%",
    minHeight: "0",
    background: cssVariableTheme.background.default,
    color: cssVariableTheme.text.primary,
    fontFamily: cssVariableTheme.typography.fontFamily,
    "& header": {
      display: "flex",
      alignItems: "center",
      flexWrap: "wrap",
      gap: cssVariableTheme.spacing.md,
      padding: `${cssVariableTheme.spacing.sm} ${cssVariableTheme.spacing.md}`,
      background: cssVariableTheme.background.paper,
      borderBottom: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
    },
    "& .title": {
      display: "flex",
      alignItems: "center",
      gap: cssVariableTheme.spacing.md,
      marginRight: "auto",
    },
    "& shade-typography": {
      margin: "0",
    },
    "& nested-route-link": {
      color: cssVariableTheme.text.secondary,
      fontSize: cssVariableTheme.typography.fontSize.sm,
    },
    "& .log": {
      flex: "1",
      minHeight: "0",
      overflow: "auto",
      margin: "0",
      padding: cssVariableTheme.spacing.md,
      fontFamily: "ui-monospace, monospace",
      fontSize: cssVariableTheme.typography.fontSize.sm,
      lineHeight: cssVariableTheme.typography.lineHeight.normal,
      whiteSpace: "pre-wrap",
      overflowWrap: "anywhere",
    },
    "& .error": { color: cssVariableTheme.palette.error.main },
    "& .warn": { color: cssVariableTheme.palette.warning.main },
    "& .info": { color: cssVariableTheme.palette.info.main },
    "& .output": { color: cssVariableTheme.text.primary },
    "& .composer": {
      display: "flex",
      alignItems: "center",
      gap: cssVariableTheme.spacing.sm,
      padding: cssVariableTheme.spacing.sm,
      borderTop: `${cssVariableTheme.shape.borderWidth} solid ${cssVariableTheme.divider}`,
      background: cssVariableTheme.background.paper,
    },
    "& .composer shade-input": {
      flex: "1",
      margin: "0",
    },
    "& .composer shade-button": {
      margin: "0",
    },
    "& .notice": {
      padding: `0 ${cssVariableTheme.spacing.md}`,
    },
  },
  render: ({ injector, useObservable, useState, useDisposable, useRef }) => {
    const access = injector.get(DeviceAccess);
    const [authorization] = useObservable("authorization", access.authorization);
    const [needsAuth] = useObservable("needs-auth", access.needsAuth);
    const [level, setLevel] = useState<ConsoleSeverity>("level", "warn");
    const [draft, setDraft] = useState("draft", "");
    const [phase, setPhase] = useState<ConsoleLinkPhase | "signed-out">("phase", "signed-out");
    const [attempt, setAttempt] = useState("attempt", 0);
    const [transcript, setTranscript] = useState<ConsoleTranscript>("transcript", createConsoleTranscript());
    const log = useRef<HTMLDivElement>("log");
    const session = useDisposable("session", () => ({
      key: "",
      generation: 0,
      link: null as ConsoleLink | null,
      transcript: createConsoleTranscript(),
      follow: true,
      [Symbol.dispose]() {
        this.generation += 1;
        this.link?.close();
      },
    }));

    const connectionKey = authorization && !needsAuth ? `${authorization}:${attempt}` : "";
    if (session.key !== connectionKey) {
      session.key = connectionKey;
      const generation = ++session.generation;
      session.link?.close();
      session.link = null;
      if (!connectionKey) {
        queueMicrotask(() => {
          if (generation === session.generation) {
            setPhase(authorization ? "unauthorized" : "signed-out");
          }
        });
      } else {
        void openConsoleLink({
          authorization,
          pageUrl: window.location.href,
          onChunk: (chunk) => {
            if (generation !== session.generation) {
              return;
            }
            session.transcript = pushConsoleChunk(session.transcript, chunk);
            setTranscript(session.transcript);
          },
          onPhase: (next) => {
            if (generation !== session.generation) {
              return;
            }
            if (next === "unauthorized") {
              access.markUnauthorized();
            }
            setPhase(next);
          },
        }).then((link) => {
          if (generation !== session.generation) {
            link.close();
            return;
          }
          session.link = link;
        });
      }
    }

    const visible = transcript.lines.filter((line) => lineVisible(line.severity, level));
    const pending = stripAnsi(transcript.pending);
    const pendingSeverity = severityOf(pending);
    const showPending = pending !== "" && lineVisible(pendingSeverity, level);
    if (log.current && session.follow) {
      log.current.scrollTop = log.current.scrollHeight;
    }

    const send = () => {
      const message = normalizeConsoleMessage(draft);
      if (!message || !session.link?.send(message)) {
        return;
      }
      setDraft("");
    };

    const chooseLevel = (value: string) => {
      if (!isConsoleSeverity(value)) {
        return;
      }
      setLevel(value);
    };

    const signedOut = !authorization || needsAuth || phase === "unauthorized";
    const connected = phase === "open";
    const canSend = connected && normalizeConsoleMessage(draft) !== null;

    return (
      <>
        <header>
          <div className="title">
            <NestedRouteLink path="/">Home</NestedRouteLink>
            <Typography variant="subtitle2">Console</Typography>
            <Typography variant="caption" color="textSecondary">
              {phaseText[phase]}
            </Typography>
          </div>
          <Typography variant="caption" color="textSecondary">
            Log level
          </Typography>
          <SegmentedControl size="small" value={level} options={levelOptions} onValueChange={chooseLevel} />
        </header>
        {signedOut ? <DeviceSignIn /> : null}
        {phase === "unreachable" || phase === "closed" ? (
          <div className="notice">
            <Alert severity="error" variant="outlined" title={phaseText[phase]}>
              <Button type="button" size="small" variant="outlined" onclick={() => setAttempt(attempt + 1)}>
                Retry
              </Button>
            </Alert>
          </div>
        ) : null}
        <div
          className="log"
          ref={log}
          onscroll={(event) => {
            const element = event.currentTarget as HTMLDivElement;
            session.follow = element.scrollHeight - element.scrollTop - element.clientHeight < 48;
          }}
        >
          {visible.map((line) => (
            <div className={line.severity ?? "output"}>{line.text.length > 0 ? line.text : " "}</div>
          ))}
          {showPending ? <div className={pendingSeverity ?? "output"}>{pending}</div> : null}
        </div>
        <form
          className="composer"
          onsubmit={(event) => {
            event.preventDefault();
          }}
        >
          <Input
            type="text"
            variant="outlined"
            size="small"
            placeholder="Command"
            maxLength={consoleMessageLimit}
            disabled={!connected}
            value={draft}
            onTextChange={setDraft}
            onkeydown={(event) => {
              if (event.key === "Enter") {
                event.preventDefault();
                send();
              }
            }}
          />
          <Button type="button" variant="contained" size="small" disabled={!canSend} onclick={send}>
            Send
          </Button>
        </form>
      </>
    );
  },
});
