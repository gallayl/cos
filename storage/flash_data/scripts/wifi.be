def show_wifi()
    var info = net.wifi()
    if !info["ok"]
        print(info["error"])
        return 1
    end
    print("mode " + info["mode"])
    print("connected " + str(info["connected"]))
    print("ssid " + info["ssid"])
    print("rssi " + str(info["rssi"]))
    print("signal " + info["signal"])
    print("auth " + info["auth"])
    print("ip " + info["ip"])
    print("ap " + info["ap_ip"])
    print("mac " + info["mac"])
    return 0
end
def main(args)
    if args.size() == 0
        return show_wifi()
    end
    if args[0] == "disconnect"
        if args.size() != 1
            print("usage: wifi disconnect")
            return 1
        end
        var result = net.wifi_disconnect()
        if result != true
            print(str(result))
            return 1
        end
        print("disconnected")
        return 0
    end
    if args[0] == "connect"
        if args.size() < 2 || args.size() > 3
            print("usage: wifi connect <ssid> [password]")
            return 1
        end
        var result = true
        if args.size() == 3
            result = net.wifi_connect(args[1], args[2])
        else
            result = net.wifi_connect(args[1])
        end
        if result != true
            print(str(result))
            return 1
        end
        print("connecting")
        return 0
    end
    print("usage: wifi [connect <ssid> [password] | disconnect]")
    return 1
end
