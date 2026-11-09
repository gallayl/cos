var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
def main(args)
    if size(args) != 1
        print("usage: wget <url>")
        return 1
    end
    var result = net.fetch(args[0], {"headers": {"Accept": "*/*"}, "timeout_ms": 10000})
    print("url " + result["url"])
    if result["status"] != 0
        print("status " + str(result["status"]))
    end
    if !result["ok"]
        print(result["error"])
    end
    if size(result["body"]) > 0
        print(result["body"])
    end
    if result["ok"]
        return 0
    end
    return 1
end
