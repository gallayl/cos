# R2/P7: repeated fetch regression/profiling.
# Run: berry exec /flash/tests/r2/repeated_fetch.be <url> [count] [text|json]
# Example: berry exec /flash/tests/r2/repeated_fetch.be http://example.com/ 5 text
# Then: berry profile  and/or  berry gc
import net

def main(args)
    if size(args) < 1
        print("usage: repeated_fetch <url> [count] [text|json]")
        return 2
    end
    var url = args[0]
    var count = 3
    if size(args) >= 2
        count = int(args[1])
    end
    var response_type = "text"
    if size(args) >= 3
        response_type = args[2]
    end
    var i = 0
    var ok_count = 0
    while i < count
        var result = net.fetch(url, {"timeout_ms": 8000, "response_type": response_type})
        if result["ok"]
            ok_count = ok_count + 1
        end
        print("fetch", i + 1, "/", count, "ok", result["ok"], "status", result["status"], "err", result["error"])
        i = i + 1
    end
    print("repeated_fetch done", ok_count, "/", count)
    print("next: berry profile; berry gc; berry profile")
    return 0
end