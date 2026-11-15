var no_network = net.ping("example.com")
var short_timeout = net.ping("example.com", 99)
var invalid_host = net.ping("bad host")
if !no_network["ok"] && no_network["error"] == "ESP_ERR_INVALID_STATE"
    log("NET_NO_WIFI_OK")
end
if !short_timeout["ok"] && short_timeout["error"] == "ESP_ERR_INVALID_ARG"
    log("NET_TIMEOUT_BOUNDS_OK")
end
if !invalid_host["ok"] && invalid_host["error"] == "ESP_ERR_INVALID_ARG"
    log("NET_INVALID_HOST_OK")
end
log("BERRY_NET_API_DONE")
def run(args)
    return 0
end
