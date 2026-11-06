function main(args: string[]): number {
  if (args.length !== 1) {
    print("usage: wget <url>");
    return 1;
  }

  const result = net.fetch(args[0], {
    headers: { Accept: "*/*" },
    timeout_ms: 10000,
  });
  print("url " + result.url);
  if (result.status !== 0) {
    print("status " + str(result.status));
  }
  if (!result.ok) {
    print(result.error);
  }
  if (result.body.length > 0) {
    print(result.body);
  }
  if (result.ok) {
    return 0;
  }
  return 1;
}
