var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
class FetchJson
    def run(args)
        if size(args) != 1
            print("usage: fetch_json <url>")
            return 1
        end
        var result = net.fetch(args[0], {"headers": {"Accept": "application/json"}, "response_type": "json", "timeout_ms": 10000})
        print("url " + result["url"])
        if result["status"] != 0
            print("status " + str(result["status"]))
        end
        if !result["ok"]
            print(result["error"])
        end
        if result["json_ok"]
            if result["json"] == nil
                print("json null")
            else
                print("json parsed")
            end
        end
        if size(result["body"]) > 0
            print(result["body"])
        end
        if result["ok"]
            return 0
        end
        return 1
    end
end

var __cos_app = FetchJson()
def main(args)
    var __cos_r = __cos_app.run(args)
    if __cos_r == nil
        return 0
    end
    return __cos_r
end
