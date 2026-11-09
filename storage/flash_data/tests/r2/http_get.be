# R2/P7: HTTP GET profiling/regression checkpoint.
# Usage from shell: berry run /flash/tests/r2/http_get.be
# Then run: berry profile
# Edit url below for your test endpoint (HTTP, not HTTPS).
import net

def main(args)
    var url = "http://example.com/"
    var result = net.fetch(url, {"timeout_ms": 8000, "response_type": "text"})
    print("http_get ok", result["ok"], "status", result["status"], "err", result["error"])
    if result["ok"]
        print("http_get body_len", size(result["body"]))
    end
    print("next: berry profile")
    return 0
end
