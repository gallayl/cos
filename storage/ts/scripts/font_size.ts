import console from "@cos/console";

export default class FontSize implements CosApplication {
  run(args: string[]): number {
    if (args.length !== 1) {
      print("usage: font_size 1|2|3");
      return 1;
    }

    const requested = args[0];
    let result: true | string = "ESP_ERR_INVALID_ARG";
    if (requested === "1") {
      result = console.set_font_size(1);
    } else if (requested === "2") {
      result = console.set_font_size(2);
    } else if (requested === "3") {
      result = console.set_font_size(3);
    } else {
      print("font size must be 1, 2, or 3");
      return 1;
    }

    if (result !== true) {
      print(str(result));
      return 1;
    }

    print("font size " + str(console.font_size()));
    return 0;
  }
}
