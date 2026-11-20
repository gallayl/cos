var __cos_mod_1 = cos_import("@cos/http")
var http = __cos_mod_1.find("default")
var __cos_mod_2 = cos_import("@cos/ui")
var ui = __cos_mod_2.find("default")
class HttpChat
    var server
    var status_label
    var messages
    var max_messages
    def init()
        self.server = nil
        self.status_label = nil
        self.messages = []
        self.max_messages = 20
    end
    def push_message(msg)
        self.messages.push(msg)
        if size(self.messages) > self.max_messages
            self.messages.remove(0)
        end
    end
    def setup(_content, width, _height)
        self.status_label = ui.label(0, "HTTP chat: starting...", 16, 16, width - 32, 24)
        var exit = ui.button(0, "EXIT", 16, 56, 88, 36)
        ui.on_click(exit, def ()
            if self.server != nil
                http.close(self.server)
            end
            ui.exit()
        end)
        var port = 8082
        self.server = http.create_server()
        http.post(self.server, "/send", def (req, res)
            var text = nil
            var from = nil
            var body = req.json
            if body != nil
                text = body["text"]
                from = body["from"]
            end
            if text == nil || str(text) == ""
                http.res_status(res, 400)
                http.res_json(res, {"ok": false, "error": "missing text"})
                return
            end
            var msg = {"text": str(text)}
            if from != nil
                msg["from"] = str(from)
            end
            self.push_message(msg)
            http.res_status(res, 200)
            http.res_json(res, {"ok": true, "count": size(self.messages)})
        end)
        http.get(self.server, "/messages", def (_req, res)
            http.res_status(res, 200)
            http.res_json(res, {"ok": true, "messages": self.messages})
        end)
        try
            http.listen(self.server, port, "0.0.0.0")
            ui.set_text(self.status_label, "HTTP chat: 0.0.0.0:" + str(port) + " /send /messages")
        except .. as e
            ui.set_text(self.status_label, "Listen failed: " + str(e))
            http.close(self.server)
        end
    end
end

var __cos_entry = HttpChat
