import http from "@cos/http";
import ui from "@cos/ui";

export default class HttpEcho implements CosUiApplication {
  server: http.Server = null as any;
  status_label: ui.LabelHandle = null as any;

  is_json_null_literal(raw: string): boolean {
    // Berry json parsing treats whitespace-surrounded "null" as valid null.
    let i = 0;
    let j = size(raw) - 1;
    while (
      i <= j &&
      ((raw as any)[i] === " " ||
        (raw as any)[i] === "\n" ||
        (raw as any)[i] === "\r" ||
        (raw as any)[i] === "\t")
    ) {
      i = i + 1;
    }
    while (
      j >= i &&
      ((raw as any)[j] === " " ||
        (raw as any)[j] === "\n" ||
        (raw as any)[j] === "\r" ||
        (raw as any)[j] === "\t")
    ) {
      j = j - 1;
    }
    if (j - i !== 3) {
      return false;
    }
    return (
      (raw as any)[i] === "n" &&
      (raw as any)[i + 1] === "u" &&
      (raw as any)[i + 2] === "l" &&
      (raw as any)[i + 3] === "l"
    );
  }

  setup(_content: unknown, width: number, _height: number): void {
    this.status_label = ui.label(0, "HTTP echo: starting...", 16, 16, width - 32, 24)!;
    const exit = ui.button(0, "EXIT", 16, 56, 88, 36)!;
    ui.on_click(exit, () => {
      if (this.server !== null) {
        http.close(this.server);
      }
      ui.exit();
    });

    const port = 8081;
    this.server = http.create_server();
    http.post(this.server, "/echo", (req, res) => {
      const body: any = req.json;
      const raw: string = str(req.text);
      if (body === null && this.is_json_null_literal(raw) === false) {
        http.res_status(res, 400);
        http.res_json(res, { error: "expected JSON body" });
        return;
      }
      http.res_status(res, 200);
      if (body !== null) {
        http.res_json(res, body);
      } else {
        http.res_set(res, "Content-Type", "application/json");
        http.res_send(res, "null");
      }
    });

    try {
      http.listen(this.server, port, "0.0.0.0");
      ui.set_text(this.status_label, "HTTP echo: 0.0.0.0:" + str(port) + " POST /echo");
    } catch (e) {
      ui.set_text(this.status_label, "Listen failed: " + str(e as any));
      http.close(this.server);
    }
  }
}
