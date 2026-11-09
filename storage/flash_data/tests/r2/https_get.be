# R2/P7: HTTPS GET profiling/regression checkpoint.
# Usage: berry run /flash/tests/r2/https_get.be
# Edit url for a known-good HTTPS endpoint. Certificate verification remains enabled.
import net

def main(args)
    var url = "https://example.com/"
    var result = net.fetch(url, {"timeout_ms": 12000, "response_type": "text"})
    print("https_get ok", result["ok"], "status", result["status"], "err", result["error"])
    if result["ok"]
        print("https_get body_len", size(result["body"]))
    end
    print("next: berry profile")
    return 0
end
