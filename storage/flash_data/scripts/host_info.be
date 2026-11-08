var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
def main(args)
    if args.size() != 0
        print("usage: host_info")
        return 1
    end
    var info = net.host()
    if !info.ok
        print(info.error)
        return 1
    end
    print("connected " + str(info.connected))
    print("ip " + info.ip)
    print("netmask " + info.netmask)
    print("gateway " + info.gateway)
    print("mac " + info.mac)
    var index = 0
    while index < info.dns.size()
        print("dns " + info.dns[index])
        index += 1
    end
    return 0
end
