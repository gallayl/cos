import net from "@cos/net";

export default class Wifi implements CosApplication {
  show_wifi(): number {
    const info = net.wifi();
    if (!info.ok) {
      print(info.error);
      return 1;
    }
    print("mode " + info.mode);
    print("connected " + str(info.connected));
    print("ssid " + info.ssid);
    print("rssi " + str(info.rssi));
    print("signal " + info.signal);
    print("auth " + info.auth);
    print("ip " + info.ip);
    print("ap " + info.ap_ip);
    print("mac " + info.mac);
    return 0;
  }

  run(args: string[]): number {
    if (args.length === 0) {
      return this.show_wifi();
    }
    if (args[0] === "disconnect") {
      if (args.length !== 1) {
        print("usage: wifi disconnect");
        return 1;
      }
      const result = net.wifi_disconnect();
      if (result !== true) {
        print(str(result));
        return 1;
      }
      print("disconnected");
      return 0;
    }
    if (args[0] === "connect") {
      if (args.length < 2 || args.length > 3) {
        print("usage: wifi connect <ssid> [password]");
        return 1;
      }
      let result: true | string = true;
      if (args.length === 3) {
        result = net.wifi_connect(args[1], args[2]);
      } else {
        result = net.wifi_connect(args[1]);
      }
      if (result !== true) {
        print(str(result));
        return 1;
      }
      print("connecting");
      return 0;
    }
    print("usage: wifi [connect <ssid> [password] | disconnect]");
    return 1;
  }
}
