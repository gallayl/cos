import system from "@cos/system";

export default class Memory implements CosApplication {
  run(args: string[]): number {
    if (args.length > 0) {
      print("Usage: memory");
      return 1;
    }
    const info = system.meminfo();
    if (info["ok"] !== true) {
      print("memory: failed (" + info["error"] + ")");
      return 1;
    }
    print("Free heap:      " + str(info["free_heap"]) + " bytes");
    print("Min free heap:  " + str(info["min_free_heap"]) + " bytes");
    print("Total heap:     " + str(info["total_heap"]) + " bytes");
    print("Max alloc:      " + str(info["max_alloc_block"]) + " bytes");
    print("Internal free:  " + str(info["internal_free_heap"]) + " bytes");
    print("Internal min:   " + str(info["internal_min_free_heap"]) + " bytes");
    print("Internal total: " + str(info["internal_total_heap"]) + " bytes");
    print("Internal max:   " + str(info["internal_max_alloc_block"]) + " bytes");
    return 0;
  }
}
