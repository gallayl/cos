import fs from "@cos/fs";

export default class ListFiles implements CosApplication {
  run(_args: string[]): number {
    const path = "/flash";
    const result: any = fs.list(path);

    if (result["ok"]) {
      const entries = result["entries"];
      let index = 0;
      while (index < result["count"]) {
        log(entries[index]["name"]);
        index += 1;
      }
      return 0;
    }
    log("Could not list " + path + ": " + result["error"]);
    return 1;
  }
}
