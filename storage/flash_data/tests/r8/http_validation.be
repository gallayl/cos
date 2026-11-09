# R8/P4+P5: HTTP worker stack/TLS measurement + timeout/abandon safety (hardware validation helper).
#
# Runs automatable checks and prints structured output. It does NOT replace the full guided checklist:
# - capture serial `R8/P4 stack: ...` logs (authoritative per-request HWM)
# - record clean-boot / BT state for HTTPS (R8/P5)
# - compare `idf.py size` when evaluating stack/buffer changes
# - use controlled endpoints for POST/max-body where needed
#
# Usage (R21 unified command script):
#   berry run /flash/tests/r8/http_validation.be [http_get_url] [https_url] [post_url] [max_body_url]
#
# Defaults:
#   http_get_url = http://example.com/
#   https_url    = https://example.com/      (may fail with TLS/PSA OOM on constrained targets; R8 records evidence)
#   post_url     = (skip)                    pass "-" to skip explicitly
#   max_body_url = (skip)
#
# After each fetch case, the script prints `net.fetch` result fields and runs:
#   berry http-stack
#   berry profile
# Capture the full serial output.

import net

var TAG = "R8"

def sep(title)
    print("")
    print("========== " + TAG + ": " + title + " ==========")
end

def run_action(cmd)
    print("$ " + cmd)
    var rc = action(cmd)
    print("rc=" + str(rc))
    return rc
end

def snap(label)
    print("--- snapshot: " + label + " ---")
    run_action("berry http-stack")
    run_action("berry profile")
end

def report_fetch(label, r)
    if r == nil
        print(label + ": result=nil")
        return
    end
    var ok = r["ok"]
    var status = r["status"]
    var err = r["error"]
    var bl = 0
    var body = r["body"]
    if body != nil
        bl = size(body)
    end
    print(label + ": ok=" + str(ok) + " status=" + str(status) + " body_len=" + str(bl) + " error=" + str(err) + " url=" + str(r["url"]))
    if r["headers"] != nil
        print(label + ": headers_count=" + str(size(r["headers"])))
    end
end

def case_fetch(label, url, opts)
    sep(label)
    print("url=" + str(url))
    print("opts=" + str(opts))
    var r = net.fetch(url, opts)
    report_fetch(label, r)
    # Allow async-ish worker completion visibility; serial R8/P4 stack log is authoritative.
    snap(label)
    return r
end

def arg_or(args, idx, default_value)
    if args != nil && size(args) > idx
        var v = args[idx]
        if v != nil && v != "-" && v != ""
            return v
        end
    end
    return default_value
end

def has_arg(args, idx)
    if args == nil || size(args) <= idx
        return false
    end
    var v = args[idx]
    return v != nil && v != "-" && v != ""
end

def main(args)
    var http_get = arg_or(args, 0, "http://example.com/")
    var https_get = arg_or(args, 1, "https://example.com/")
    var post_url = nil
    var max_url = nil
    if has_arg(args, 2)
        post_url = args[2]
    end
    if has_arg(args, 3)
        max_url = args[3]
    end

    var post_desc = "(skip)"
    var max_desc = "(skip)"
    if post_url != nil
        post_desc = post_url
    end
    if max_url != nil
        max_desc = max_url
    end

    sep("START")
    print("R8 hardware validation helper")
    print("http_get=" + http_get)
    print("https_get=" + https_get)
    print("post_url=" + post_desc)
    print("max_url=" + max_desc)
    print("NOTE: capture serial output including 'R8/P4 stack:' lines. HTTPS may fail on constrained targets; record evidence, keep cert verification enabled.")
    run_action("wifi status")
    print("If Wi-Fi is not connected, net.fetch typically returns ESP_ERR_INVALID_STATE before starting the HTTP worker.")
    run_action("mem")
    snap("baseline")

    # P4 matrix (network-dependent). Use clean boot / stable Wi-Fi for comparable measurements.
    case_fetch("HTTP GET small", http_get, {"timeout_ms": 10000, "total_timeout_ms": 15000, "method": "GET"})

    if post_url != nil
        case_fetch("HTTP POST body", post_url, {
            "method": "POST",
            "timeout_ms": 10000,
            "total_timeout_ms": 15000,
            "headers": {"Content-Type": "application/json", "Accept": "application/json"},
            "body": "{\"r8\":\"post\",\"n\":1}"
        })
    else
        sep("HTTP POST body")
        print("SKIP: no post_url argument. Pass a controlled echo endpoint as arg 3 to test POST+body.")
    end

    # P5: HTTPS (do not disable verification). Known R2 follow-up: PSA/X.509 OOM (-141) possible.
    case_fetch("HTTPS GET", https_get, {"timeout_ms": 15000, "total_timeout_ms": 20000, "method": "GET"})
    run_action("berry gc")
    snap("after HTTPS + gc")

    if max_url != nil
        case_fetch("Max/large response", max_url, {"timeout_ms": 15000, "total_timeout_ms": 20000, "method": "GET"})
    else
        sep("Max/large response")
        print("SKIP: no max_body_url argument. Pass an endpoint returning a body near NETWORK_HTTP_BODY_MAX (2048) as arg 4.")
        print("Expected: oversized responses may fail with ESP_ERR_INVALID_SIZE; record stack/heap.")
    end

    # Timeout/abandon safety (R8 software fix).
    sep("Timeout/abandon safety")
    print("Triggering short timeout to unreachable address (may take ~timeout; caller should get timeout/empty snapshot).")
    var t = net.fetch("http://10.255.255.1/r8-timeout", {"timeout_ms": 100, "total_timeout_ms": 200, "method": "GET"})
    report_fetch("timeout", t)
    print("Immediate follow-up while worker may still be busy (expect ESP_ERR_INVALID_STATE or equivalent if busy):")
    var t2 = net.fetch(http_get, {"timeout_ms": 5000, "total_timeout_ms": 8000, "method": "GET"})
    report_fetch("immediate_followup", t2)
    print("Retry after waiting: run this script again or run a manual fetch later if follow-up was busy.")
    snap("after timeout sequence")

    # Repeated requests (heap stability / cleanup). Keep count small to limit runtime.
    sep("Repeated HTTP GET (heap stability sample)")
    var i = 0
    while i < 3
        var rr = net.fetch(http_get, {"timeout_ms": 10000, "total_timeout_ms": 15000, "method": "GET"})
        report_fetch("repeat[" + str(i) + "]", rr)
        i = i + 1
    end
    run_action("berry gc")
    snap("after repeated + gc")

    sep("DONE")
    print("R8 helper finished. Manual/hardware evidence still required:")
    print("- P4: record HWM for GET/POST/HTTPS/max/timeout/error (>=3x for worst cases), account for StackType_t unit, apply >=25% margin rule before changing HTTP_TASK_STACK.")
    print("- P5: HTTPS heap/largest/watermark on clean boot with cert verification enabled; record PSA/X.509 -141 if seen (endpoint/boot/BT state).")
    print("- Timeout safety: confirm no crash/stale data; busy follow-up -> ESP_ERR_INVALID_STATE; later success OK.")
    print("- R2 completion: repeated HTTP 200, HTTPS success on valid endpoint, large/max body behavior, no progressive heap decrease after GC.")
    return 0
end
