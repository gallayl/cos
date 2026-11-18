var __cos_mod_1 = cos_import("@cos/ui")
var ui = __cos_mod_1.find("default")
class AsyncDemo
    var status_label
    def setup(_content, width, _height)
        self.status_label = ui.label(0, "Async demo: starting...", 16, 16, width - 32, 24)
        var exit = ui.button(0, "EXIT", 16, 56, 88, 36)
        ui.on_click(exit, def ()
            ui.exit()
        end)
        self.run_demo()
    end
    def run_demo()
        return Promise.resolve(nil).then(def(_cos_async_init)
            return (system.sleep(1000)).then(def(_cos_await_v)
                ui.set_text(self.status_label, "Async demo: slept 1000ms")
                return (system.sleep(1000)).then(def(_cos_await_v)
                    ui.set_text(self.status_label, "Async demo: done")
                    return nil
                end)
            end)
        end)
    end
end

var __cos_entry = AsyncDemo
