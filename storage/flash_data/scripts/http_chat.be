var __cos_mod_1 = cos_import("@cos/http")
var http = __cos_mod_1.find("default")
class HttpChat
    var messages
    var max_messages
    def init()
        self.messages = []
        self.max_messages = 20
    end
    def push_message(msg)
        self.messages.push(msg)
        if size(self.messages) > self.max_messages
            self.messages.remove(0)
        end
    end
    def run(args)
        var port = 8082
        if size(args) > 1
            print("usage: http_chat [port]")
            return 1
        end
        if size(args) == 1
            var parsed = 0
            try
                parsed = int(args[0])
            except ..
                print("invalid port")
                return 1
            end
            if str(parsed) != args[0] || parsed <= 0 || parsed > 65535
                print("invalid port")
                return 1
            end
            port = parsed
        end
        var server = http.create_server()
        http.post(server, "/send", def (req, res)
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
        http.get(server, "/messages", def (_req, res)
            http.res_status(res, 200)
            http.res_json(res, {"ok": true, "messages": self.messages})
        end)
        try
            http.listen(server, port, "0.0.0.0")
            print("http_chat listening on 0.0.0.0:" + str(port))
            print("POST /send {\"text\":\"hi\"}; GET /messages")
            return 0
        except .. as e
            print("http_chat listen failed: " + str(e))
            http.close(server)
            return 1
        end
    end
end

var __cos_entry = HttpChat()
