import light from "@cos/light";

export default class Light implements CosApplication {
  run(_args: string[]): number {
    const result: any = light.read();
    if (result["ok"] === true) {
      print("Light sensor: " + str(result["value"]));
      return 0;
    }
    print("light: read failed");
    return 1;
  }
}
