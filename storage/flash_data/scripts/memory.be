var __cos_mod_1 = cos_import("@cos/system")
var system = __cos_mod_1.find("default")
class Memory
    def run(args)
        if size(args) > 0
            print("Usage: memory")
            return 1
        end
        var info = system.meminfo()
        if info["ok"] != true
            print("memory: failed (" + info["error"] + ")")
            return 1
        end
        print("Free heap:      " + str(info["free_heap"]) + " bytes")
        print("Min free heap:  " + str(info["min_free_heap"]) + " bytes")
        print("Total heap:     " + str(info["total_heap"]) + " bytes")
        print("Max alloc:      " + str(info["max_alloc_block"]) + " bytes")
        print("Internal free:  " + str(info["internal_free_heap"]) + " bytes")
        print("Internal min:   " + str(info["internal_min_free_heap"]) + " bytes")
        print("Internal total: " + str(info["internal_total_heap"]) + " bytes")
        print("Internal max:   " + str(info["internal_max_alloc_block"]) + " bytes")
        return 0
    end
end

var __cos_entry = Memory()
