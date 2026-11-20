import http from "@cos/http";

export default class HttpChat implements CosApplication {
  messages: any = [];
  max_messages: number = 20;

  push_message(msg: any): void {
    this.messages.push(msg);
    if (size(this.messages) > this.max_messages) {
      this.messages.remove(0);
    }
  }

  run(args: string[]): number {
    let port = 8082;
    if (args.length > 1) {
      print("usage: http_chat [port]");
      return 1;
    }
    if (args.length === 1) {
      let parsed = 0;
      try {
        parsed = int(args[0]);
      } catch {
        print("invalid port");
        return 1;
      }
      if (str(parsed) !== args[0] || parsed <= 0 || parsed > 65535) {
        print("invalid port");
        return 1;
      }
      port = parsed;
    }

    const server = http.create_server();

    http.post(server, "/send", (req, res) => {
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

    http.get(server, "/messages", (_req, res) => {
      http.res_status(res, 200);
      http.res_json(res, { ok: true, messages: this.messages });
    });

    try {
      http.listen(server, port, "0.0.0.0");
      print("http_chat listening on 0.0.0.0:" + str(port));
      print("POST /send {\"text\":\"hi\"}; GET /messages");
      return 0;
    } catch (e) {
      print("http_chat listen failed: " + str(e as any));
      http.close(server);
      return 1;
    }
  }
}
