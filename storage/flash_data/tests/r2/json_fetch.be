# R2/P7: HTTP(S) JSON fetch regression. Edit url for an endpoint returning JSON.
import net

def main(args)
    var url = "http://example.com/"
    var result = net.fetch(url, {"timeout_ms": 12000, "response_type": "json"})
    print("json_fetch ok", result["ok"], "status", result["status"], "json_ok", result["json_ok"], "err", result["error"])
    if result["json_ok"]
        print("json_fetch json", result["json"])
    end
    print("next: berry profile")
    return 0
end
