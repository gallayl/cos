import system from "@cos/system";

export default class Time implements CosApplication {
  run(args: string[]): number {
    const info = system.time_info();
    if (info["ok"] !== true) {
      print("time: format error (" + info["error"] + ")");
      return 1;
    }

    if (args.length >= 1 && args[0] === "utc") {
      print(info["utc"]);
      return 0;
    }

    if (args.length >= 1 && args[0] === "epoch") {
      print(str(info["epoch"]));
      return 0;
    }

    let synced = "no";
    if (info["synced"] === true) {
      synced = "yes";
    }

    print("Local:  " + info["local"]);
    print("UTC:    " + info["utc"]);
    print("Synced: " + synced);
    return 0;
  }
}
