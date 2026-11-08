var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
def main(args)
    if args.size() != 1
        print("usage: fetch_json <url>")
        return 1
    end
    var result = net.fetch(args[0], {"headers": {"Accept": "application/json"}, "response_type": "json", "timeout_ms": 10000})
    print("url " + result.url)
    if result.status != 0
        print("status " + str(result.status))
    end
    if !result.ok
        print(result.error)
    end
    if result.json_ok
        if result.json == nil
            print("json null")
        else
            print("json parsed")
        end
    end
    if result.body.size() > 0
        print(result.body)
    end
    if result.ok
        return 0
    end
    return 1
end
