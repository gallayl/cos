var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
var __cos_mod_2 = cos_import("@cos/ui")
var ui = __cos_mod_2.find("default")
class Onboarding
    var screen
    var width
    var height
    def setup(_content, width, height)
        self.width = width
        self.height = height
        self.screen = 1
        self.render()
    end
    def render()
        ui.clear(0)
        if self.screen == 1
            self.renderGreeting()
        elif self.screen == 2
            self.renderWifiStatus()
        else
            self.renderTips()
        end
    end
    def renderGreeting()
        var w = self.width
        var h = self.height
        ui.label(0, "Welcome to COS", 16, 16, w - 32, 24)
        ui.label(0, "Onboarding demo (3 screens)", 16, 44, w - 32, 20)
        ui.label(0, "This template shows a small app structure:", 16, 76, w - 32, 20)
        ui.label(0, "- explicit screen state", 24, 100, w - 48, 20)
        ui.label(0, "- rebuild UI per screen", 24, 122, w - 48, 20)
        ui.label(0, "- read-only Wi-Fi status + guidance", 24, 144, w - 48, 20)
        ui.label(0, "No data is saved (demo-only).", 16, 176, w - 32, 20)
        var next = ui.button(0, "Next", w - 112, h - 44, 96, 32)
        ui.on_click(next, def ()
            self.goTo(2)
        end)
    end
    def renderWifiStatus()
        var w = self.width
        var h = self.height
        ui.label(0, "Wi-Fi status (read-only)", 16, 12, w - 32, 22)
        var statusLines = self.wifiStatusLines()
        var y = 38
        var i = 0
        while i < size(statusLines) && y < 126
            ui.label(0, statusLines[i], 16, y, w - 32, 18)
            y += 20
            i += 1
        end
        ui.label(0, "Configure via shell/scripts, e.g.:", 16, 132, w - 32, 18)
        ui.label(0, "  wifi", 16, 150, w - 32, 18)
        ui.label(0, "  net.wifi_connect(ssid, password)", 16, 168, w - 32, 18)
        ui.label(0, "No scan/password UI in this demo.", 16, 188, w - 32, 18)
        var back = ui.button(0, "Back", 16, h - 44, 88, 32)
        var next = ui.button(0, "Next", w - 112, h - 44, 96, 32)
        ui.on_click(back, def ()
            self.goTo(1)
        end)
        ui.on_click(next, def ()
            self.goTo(3)
        end)
    end
    def renderTips()
        var w = self.width
        var h = self.height
        ui.label(0, "Tips and exit", 16, 12, w - 32, 22)
        ui.label(0, "- Keep widget trees small and explicit.", 16, 40, w - 32, 18)
        ui.label(0, "- Use in-memory screen state; rebuild on change.", 16, 60, w - 32, 18)
        ui.label(0, "- Clean up timers/callbacks you own on exit.", 16, 80, w - 32, 18)
        ui.label(0, "- Expect limited RAM/stack; avoid large locals.", 16, 100, w - 32, 18)
        ui.label(0, "- net status can be unavailable; show guidance.", 16, 120, w - 32, 18)
        ui.label(0, "Press Done to close (ui.exit).", 16, 148, w - 32, 18)
        var back = ui.button(0, "Back", 16, h - 44, 88, 32)
        var done = ui.button(0, "Done", w - 112, h - 44, 96, 32)
        ui.on_click(back, def ()
            self.goTo(2)
        end)
        ui.on_click(done, def ()
            self.exitApp()
        end)
    end
    def wifiStatusLines()
        var lines = []
        var wifiOk = false
        var hostOk = false
        try
            var w = net.wifi()
            if w.ok
                wifiOk = true
                var mode = w.mode
                if mode == ""
                    mode = "?"
                end
                var conn = "no"
                if w.connected
                    conn = "yes"
                end
                lines.push("Connected: " + conn + "  Mode: " + mode)
                if w.connected && w.ssid != ""
                    lines.push("SSID: " + w.ssid)
                elif !w.connected
                    lines.push("SSID: (not associated)")
                end
                if w.ip != ""
                    lines.push("IP: " + w.ip)
                end
            else
                lines.push("Wi-Fi status: unavailable (" + w.error + ")")
            end
        except .. as _e
            lines.push("Wi-Fi status: unavailable (error)")
        end
        try
            var h = net.host()
            if h.ok
                hostOk = true
                if h.ip != ""
                    lines.push("Host IP: " + h.ip)
                elif wifiOk
                    lines.push("Host IP: (none)")
                end
            elif !wifiOk
                lines.push("Host info: unavailable (" + h.error + ")")
            end
        except .. as _e
            if !wifiOk
                lines.push("Host info: unavailable (error)")
            end
        end
        if !wifiOk && !hostOk && size(lines) == 0
            lines.push("Network status unavailable.")
            lines.push("Use shell commands to inspect/connect.")
        end
        return lines
    end
    def goTo(screen)
        self.screen = screen
        self.render()
    end
    def exitApp()
        log("Onboarding: exit requested")
        ui.clear(0)
        ui.exit()
    end
end

var __cos_entry = Onboarding
