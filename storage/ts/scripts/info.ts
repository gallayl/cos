import fs from "@cos/fs";

export default class Info implements CosApplication {
  run(_args: string[]): number {
    const info: any = fs.storage_info();
    print("Flash (LittleFS):");
    print("  Total: " + str(int(info["flash_total"])) + " bytes");
    print("  Used:  " + str(int(info["flash_used"])) + " bytes");
    if (info["sd_mounted"]) {
      print("SD card (" + info["sd_type"] + "):");
      print("  Total: " + str(int(info["sd_total"])) + " bytes");
      print("  Used:  " + str(int(info["sd_used"])) + " bytes");
    } else {
      print("SD card: not mounted");
    }
    return 0;
  }
}
