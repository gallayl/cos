import { describe, expect, it, vi } from "vitest";

import { openConsoleLink, consoleWebSocketUrl, type ConsoleSocket } from "./console-link";

const authorization = `Basic ${btoa("cos:cos12345")}`;

class FakeSocket implements ConsoleSocket {
  static latest: FakeSocket | undefined;
  readyState = 0;
  onopen: ConsoleSocket["onopen"] = null;
  onmessage: ConsoleSocket["onmessage"] = null;
  onerror: ConsoleSocket["onerror"] = null;
  onclose: ConsoleSocket["onclose"] = null;
  sent: string[] = [];
  url: string;

  constructor(url: string) {
    this.url = url;
    FakeSocket.latest = this;
  }

  send(data: string) {
    this.sent.push(data);
  }

  close() {
    this.readyState = 3;
    this.onclose?.({} as CloseEvent);
  }
}

describe("console link", () => {
  it("puts the device credentials in the websocket URL", () => {
    expect(consoleWebSocketUrl("http://localhost:5173/console", authorization)).toBe(
      "ws://cos:cos12345@localhost:5173/ws",
    );
    expect(consoleWebSocketUrl("https://cos.local/console", authorization)).toBe("wss://cos:cos12345@cos.local/ws");
    expect(consoleWebSocketUrl("http://localhost/", "")).toBeNull();
  });

  it("does not open a socket when the device rejects the credentials", async () => {
    FakeSocket.latest = undefined;
    const phases: string[] = [];
    const fetchImpl = vi.fn(async () => new Response("Unauthorized", { status: 401 }));
    const link = await openConsoleLink({
      authorization,
      pageUrl: "http://localhost/",
      fetchImpl,
      webSocket: FakeSocket,
      onChunk: () => {},
      onPhase: (phase) => phases.push(phase),
    });

    expect(fetchImpl).toHaveBeenCalledOnce();
    expect(FakeSocket.latest).toBeUndefined();
    expect(phases).toEqual(["connecting", "unauthorized"]);
    expect(link.send("help")).toBe(false);
  });

  it("forwards one text frame and reports socket output", async () => {
    const chunks: string[] = [];
    const phases: string[] = [];
    const fetchImpl = vi.fn(async () => new Response("200000", { status: 200 }));
    const link = await openConsoleLink({
      authorization,
      pageUrl: "http://localhost/console",
      fetchImpl,
      webSocket: FakeSocket,
      onChunk: (chunk) => chunks.push(chunk),
      onPhase: (phase) => phases.push(phase),
    });

    const socket = FakeSocket.latest;
    expect(socket?.url).toBe("ws://cos:cos12345@localhost/ws");
    socket!.readyState = 1;
    socket!.onopen?.({} as Event);
    socket!.onmessage?.({ data: "I (1) main: ready\n" } as MessageEvent);
    expect(link.send("help")).toBe(true);
    expect(socket!.sent).toEqual(["help"]);
    expect(chunks).toEqual(["I (1) main: ready\n"]);
    expect(phases).toEqual(["connecting", "open"]);
  });
});
