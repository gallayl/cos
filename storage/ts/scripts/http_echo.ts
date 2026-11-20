import http from "@cos/http";

export default class HttpEcho implements CosApplication {
  run(args: string[]): number {
    let port = 8081;
    if (args.length > 1) {
      print("usage: http_echo [port]");
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
    http.post(server, "/echo", (req, res) => {
      const body: any = req.json;
      if (body === null) {
        http.res_status(res, 400);
        http.res_json(res, { error: "expected JSON body" });
        return;
      }
      http.res_status(res, 200);
      http.res_json(res, body);
    });

    try {
      http.listen(server, port, "0.0.0.0");
      print("http_echo listening on 0.0.0.0:" + str(port));
      print("POST /echo with JSON body");
      return 0;
    } catch (e) {
      print("http_echo listen failed: " + str(e as any));
      http.close(server);
      return 1;
    }
  }
}
