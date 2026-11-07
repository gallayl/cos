def main(args)
    if args.size() != 1
        print("usage: wget <url>")
        return 1
    end
    var result = net.fetch(args[0], {"headers": {"Accept": "*/*"}, "timeout_ms": 10000})
    print("url " + result["url"])
    if result["status"] != 0
        print("status " + str(result["status"]))
    end
    if !result["ok"]
        print("Request failed")
        print(result["error"])
        return 1
    end
    if size(result["body"]) > 0
        print(result["body"])
        return 0
    end
    print(result)
    return 1
end
