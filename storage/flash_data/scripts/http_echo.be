var __cos_mod_1 = cos_import("@cos/http")
var http = __cos_mod_1.find("default")
class HttpEcho
    def run(args)
        var port = 8081
        if size(args) > 1
            print("usage: http_echo [port]")
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
        http.post(server, "/echo", def (req, res)
            var body = req.json
            if body == nil
                http.res_status(res, 400)
                http.res_json(res, {"error": "expected JSON body"})
                return
            end
            http.res_status(res, 200)
            http.res_json(res, body)
        end)
        try
            http.listen(server, port, "0.0.0.0")
            print("http_echo listening on 0.0.0.0:" + str(port))
            print("POST /echo with JSON body")
            return 0
        except .. as e
            print("http_echo listen failed: " + str(e))
            http.close(server)
            return 1
        end
    end
end

var __cos_entry = HttpEcho()
