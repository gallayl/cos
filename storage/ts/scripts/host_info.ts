function main(args: string[]): number {
  if (args.length !== 0) {
    print("usage: host_info");
    return 1;
  }

  const info = net.host();
  if (!info.ok) {
    print(info.error);
    return 1;
  }

  print("connected " + str(info.connected));
  print("ip " + info.ip);
  print("netmask " + info.netmask);
  print("gateway " + info.gateway);
  print("mac " + info.mac);
  let index = 0;
  while (index < info.dns.length) {
    print("dns " + info.dns[index]);
    index += 1;
  }
  return 0;
}
