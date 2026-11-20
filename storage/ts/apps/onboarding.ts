import net from "@cos/net";
import ui from "@cos/ui";

/**
 * Demo-only onboarding template (R20d): 3 in-memory screens showing a small,
 * explicit navigation pattern with read-only Wi-Fi status/guidance.
 * No persistence, scanning, or password entry (current API scope).
 */
export default class Onboarding implements CosUiApplication {
  screen!: number;
  width!: number;
  height!: number;

  setup(_content: unknown, width: number, height: number): void {
    this.width = width;
    this.height = height;
    this.screen = 1;
    this.render();
  }

  render(): void {
    ui.clear(0);
    if (this.screen === 1) {
      this.renderGreeting();
    } else if (this.screen === 2) {
      this.renderWifiStatus();
    } else {
      this.renderTips();
    }
  }

  renderGreeting(): void {
    const w = this.width;
    const h = this.height;
    ui.label(0, "Welcome to COS", 16, 16, w - 32, 24);
    ui.label(0, "Onboarding demo (3 screens)", 16, 44, w - 32, 20);
    ui.label(0, "This template shows a small app structure:", 16, 76, w - 32, 20);
    ui.label(0, "- explicit screen state", 24, 100, w - 48, 20);
    ui.label(0, "- rebuild UI per screen", 24, 122, w - 48, 20);
    ui.label(0, "- read-only Wi-Fi status + guidance", 24, 144, w - 48, 20);
    ui.label(0, "No data is saved (demo-only).", 16, 176, w - 32, 20);

    const next = ui.button(0, "Next", w - 112, h - 44, 96, 32)!;
    ui.on_click(next, () => {
      this.goTo(2);
    });
  }

  renderWifiStatus(): void {
    const w = this.width;
    const h = this.height;
    ui.label(0, "Wi-Fi status (read-only)", 16, 12, w - 32, 22);

    const statusLines = this.wifiStatusLines();
    let y = 38;
    let i = 0;
    while (i < statusLines.length && y < 126) {
      ui.label(0, statusLines[i]!, 16, y, w - 32, 18);
      y += 20;
      i += 1;
    }

    ui.label(0, "Configure via shell/scripts, e.g.:", 16, 132, w - 32, 18);
    ui.label(0, "  wifi", 16, 150, w - 32, 18);
    ui.label(0, "  net.wifi_connect(ssid, password)", 16, 168, w - 32, 18);
    ui.label(0, "No scan/password UI in this demo.", 16, 188, w - 32, 18);

    const back = ui.button(0, "Back", 16, h - 44, 88, 32)!;
    const next = ui.button(0, "Next", w - 112, h - 44, 96, 32)!;
    ui.on_click(back, () => {
      this.goTo(1);
    });
    ui.on_click(next, () => {
      this.goTo(3);
    });
  }

  renderTips(): void {
    const w = this.width;
    const h = this.height;
    ui.label(0, "Tips and exit", 16, 12, w - 32, 22);
    ui.label(0, "- Keep widget trees small and explicit.", 16, 40, w - 32, 18);
    ui.label(0, "- Use in-memory screen state; rebuild on change.", 16, 60, w - 32, 18);
    ui.label(0, "- Clean up timers/callbacks you own on exit.", 16, 80, w - 32, 18);
    ui.label(0, "- Expect limited RAM/stack; avoid large locals.", 16, 100, w - 32, 18);
    ui.label(0, "- net status can be unavailable; show guidance.", 16, 120, w - 32, 18);
    ui.label(0, "Press Done to close (ui.exit).", 16, 148, w - 32, 18);

    const back = ui.button(0, "Back", 16, h - 44, 88, 32)!;
    const done = ui.button(0, "Done", w - 112, h - 44, 96, 32)!;
    ui.on_click(back, () => {
      this.goTo(2);
    });
    ui.on_click(done, () => {
      this.exitApp();
    });
  }

  wifiStatusLines(): string[] {
    const lines: string[] = [];
    let wifiOk = false;
    let hostOk = false;

    try {
      const w = net.wifi();
      if (w.ok) {
        wifiOk = true;
        let mode = w.mode;
        if (mode === "") {
          mode = "?";
        }
        let conn = "no";
        if (w.connected) {
          conn = "yes";
        }
        lines.push("Connected: " + conn + "  Mode: " + mode);
        if (w.connected && w.ssid !== "") {
          lines.push("SSID: " + w.ssid);
        } else if (!w.connected) {
          lines.push("SSID: (not associated)");
        }
        if (w.ip !== "") {
          lines.push("IP: " + w.ip);
        }
      } else {
        lines.push("Wi-Fi status: unavailable (" + w.error + ")");
      }
    } catch (_e) {
      lines.push("Wi-Fi status: unavailable (error)");
    }

    try {
      const h = net.host();
      if (h.ok) {
        hostOk = true;
        if (h.ip !== "") {
          lines.push("Host IP: " + h.ip);
        } else if (wifiOk) {
          lines.push("Host IP: (none)");
        }
      } else if (!wifiOk) {
        lines.push("Host info: unavailable (" + h.error + ")");
      }
    } catch (_e) {
      if (!wifiOk) {
        lines.push("Host info: unavailable (error)");
      }
    }

    if (!wifiOk && !hostOk && lines.length === 0) {
      lines.push("Network status unavailable.");
      lines.push("Use shell commands to inspect/connect.");
    }
    return lines;
  }

  goTo(screen: number): void {
    this.screen = screen;
    this.render();
  }

  exitApp(): void {
    log("Onboarding: exit requested");
    ui.clear(0);
    ui.exit();
  }
}
