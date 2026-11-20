import http from "@cos/http";
import ui from "@cos/ui";

export default class HttpChat implements CosUiApplication {
  server: http.Server = null as any;
  status_label: ui.LabelHandle = null as any;
  messages: any = [];
  max_messages: number = 20;

  push_message(msg: any): void {
    this.messages.push(msg);
    if (size(this.messages) > this.max_messages) {
      this.messages.remove(0);
    }
  }

  setup(_content: unknown, width: number, _height: number): void {
    this.status_label = ui.label(0, "HTTP chat: starting...", 16, 16, width - 32, 24)!;
    const exit = ui.button(0, "EXIT", 16, 56, 88, 36)!;
    ui.on_click(exit, () => {
      if (this.server !== null) {
        http.close(this.server);
      }
      ui.exit();
    });

    const port = 8082;
    this.server = http.create_server();

    http.post(this.server, "/send", (req, res) => {
      let text: any = null;
      let from: any = null;
      const body: any = req.json;
      if (body !== null) {
        text = body["text"];
        from = body["from"];
      }
      if (text === null || str(text) === "") {
        http.res_status(res, 400);
        http.res_json(res, { ok: false, error: "missing text" });
        return;
      }

      let msg: any = { text: str(text) };
      if (from !== null) {
        msg["from"] = str(from);
      }
      this.push_message(msg);
      http.res_status(res, 200);
      http.res_json(res, { ok: true, count: size(this.messages) });
    });

    http.get(this.server, "/messages", (_req, res) => {
      http.res_status(res, 200);
      http.res_json(res, { ok: true, messages: this.messages });
    });

    try {
      http.listen(this.server, port, "0.0.0.0");
      ui.set_text(this.status_label, "HTTP chat: 0.0.0.0:" + str(port) + " /send /messages");
    } catch (e) {
      ui.set_text(this.status_label, "Listen failed: " + str(e as any));
      http.close(this.server);
    }
  }
}
