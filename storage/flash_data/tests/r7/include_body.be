# R7/P3: net.fetch include_body option.
# Usage (command script; pass args with `berry exec`):
#   berry exec /sdcard/tests/r7/include_body.be                  # arg-validation only (no network)
#   berry exec /sdcard/tests/r7/include_body.be <json-url>       # also checks live JSON fetch behavior
#
# Expected arg-validation: prints R7 ARG OK.
# Expected live check with a URL returning JSON: body omitted when include_body=false,
# body present by default; prints R7 OK when all checks pass.

import net

def fail(msg)
    print("R7 FAIL:", msg)
    return false
end

def main(args)
    # 1) Invalid include_body type must be rejected before making the request.
    var bad = net.fetch("http://127.0.0.1/r7-invalid-include-body", {
        "response_type": "json",
        "include_body": "no"
    })
    if bad == nil
        fail("bad result nil")
        return 1
    end
    if bad["ok"] != false || bad["error"] != "ESP_ERR_INVALID_ARG"
        fail("invalid include_body type should return ESP_ERR_INVALID_ARG, got ok=" + str(bad["ok"]) + " error=" + str(bad["error"]))
        return 1
    end
    print("R7 arg validation: invalid include_body type rejected")

    # 2) Valid bools are accepted syntactically. The request may fail due to network;
    # that is fine for offline validation as long as it is not ESP_ERR_INVALID_ARG.
    var accepted_false = net.fetch("http://127.0.0.1/r7-bool", {"include_body": false, "timeout_ms": 100, "total_timeout_ms": 100})
    if accepted_false["error"] == "ESP_ERR_INVALID_ARG"
        fail("include_body false was rejected as invalid arg")
        return 1
    end
    var accepted_true = net.fetch("http://127.0.0.1/r7-bool", {"include_body": true, "timeout_ms": 100, "total_timeout_ms": 100})
    if accepted_true["error"] == "ESP_ERR_INVALID_ARG"
        fail("include_body true was rejected as invalid arg")
        return 1
    end
    print("R7 arg validation: bool values accepted")

    if size(args) < 1
        print("R7 ARG OK (no URL provided; network behavior not checked)")
        print("next: berry exec /sdcard/tests/r7/include_body.be https://httpbin.org/json")
        return 0
    end

    var url = args[0]
    print("R7 live URL:", url)

    # 3) Default behavior keeps raw body.
    var with_body = net.fetch(url, {"timeout_ms": 12000, "response_type": "json"})
    print("with_body ok", with_body["ok"], "status", with_body["status"], "json_ok", with_body["json_ok"], "body_len", size(with_body["body"]), "err", with_body["error"])
    if with_body["ok"] != true || with_body["json_ok"] != true
        fail("default JSON fetch did not succeed/parse; check network/URL")
        return 1
    end
    if size(with_body["body"]) <= 0
        fail("default fetch should include raw body")
        return 1
    end

    # 4) include_body=false omits raw body string while keeping parsed JSON.
    var no_body = net.fetch(url, {"timeout_ms": 12000, "response_type": "json", "include_body": false})
    print("no_body ok", no_body["ok"], "status", no_body["status"], "json_ok", no_body["json_ok"], "body_len", size(no_body["body"]), "err", no_body["error"])
    if no_body["ok"] != true || no_body["json_ok"] != true
        fail("include_body=false JSON fetch did not succeed/parse")
        return 1
    end
    if size(no_body["body"]) != 0
        fail("include_body=false should return empty body, got len=" + str(size(no_body["body"])))
        return 1
    end
    if no_body["json"] == nil
        fail("include_body=false should still return parsed json")
        return 1
    end

    print("R7 OK")
    print("next: berry profile")
    return 0
end
