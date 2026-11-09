var __cos_mod_1 = cos_import("@cos/net")
var net = __cos_mod_1.find("default")
class HostInfo
    def run(args)
        if size(args) != 0
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
        while index < size(info.dns)
            print("dns " + info.dns[index])
            index += 1
        end
        return 0
    end
end

var __cos_app = HostInfo()
def main(args)
    var __cos_r = __cos_app.run(args)
    if __cos_r == nil
        return 0
    end
    return __cos_r
end
if !(_cos_cmd_exec == true)
    main([])
end
