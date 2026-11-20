var __cos_mod_1 = cos_import("@cos/http")
var http = __cos_mod_1.find("default")
var __cos_mod_2 = cos_import("@cos/ui")
var ui = __cos_mod_2.find("default")
class HttpEcho
    var server
    var status_label
    def init()
        self.server = nil
        self.status_label = nil
    end
    def is_json_null_literal(raw)
        var i = 0
        var j = size(raw) - 1
        while i <= j && ((raw)[i] == " " || (raw)[i] == "\n" || (raw)[i] == "\r" || (raw)[i] == "\t")
            i = i + 1
        end
        while j >= i && ((raw)[j] == " " || (raw)[j] == "\n" || (raw)[j] == "\r" || (raw)[j] == "\t")
            j = j - 1
        end
        if j - i != 3
            return false
        end
        return ((raw)[i] == "n" && (raw)[i + 1] == "u" && (raw)[i + 2] == "l" && (raw)[i + 3] == "l")
    end
    def setup(_content, width, _height)
        self.status_label = ui.label(0, "HTTP echo: starting...", 16, 16, width - 32, 24)
        var exit = ui.button(0, "EXIT", 16, 56, 88, 36)
        ui.on_click(exit, def ()
            if self.server != nil
                http.close(self.server)
            end
            ui.exit()
        end)
        var port = 8081
        self.server = http.create_server()
        http.post(self.server, "/echo", def (req, res)
            var body = req.json
            var raw = str(req.text)
            if body == nil && self.is_json_null_literal(raw) == false
                http.res_status(res, 400)
                http.res_json(res, {"error": "expected JSON body"})
                return
            end
            http.res_status(res, 200)
            if body != nil
                http.res_json(res, body)
            else
                http.res_set(res, "Content-Type", "application/json")
                http.res_send(res, "null")
            end
        end)
        try
            http.listen(self.server, port, "0.0.0.0")
            ui.set_text(self.status_label, "HTTP echo: 0.0.0.0:" + str(port) + " POST /echo")
        except .. as e
            ui.set_text(self.status_label, "Listen failed: " + str(e))
            http.close(self.server)
        end
    end
end

var __cos_entry = HttpEcho
