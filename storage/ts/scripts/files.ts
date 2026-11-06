function main(args: string[]): number {
  if (args.length !== 0) {
    print("usage: files");
    return 1;
  }

  const textPath = "./sample.txt";
  const jsonPath = "./sample.json";
  const wroteText = fs.write_text(textPath, "hello\n");
  if (wroteText !== true) {
    print(str(wroteText));
    return 1;
  }

  const text = fs.read_text(textPath);
  if (!text.ok) {
    print(text.error);
    return 1;
  }
  print(text.text);

  const wroteJson = fs.write_json(jsonPath, {
    label: "cos",
    count: 2,
    flags: [true, false],
    child: { ok: true },
  });
  if (wroteJson !== true) {
    print(str(wroteJson));
    return 1;
  }

  const loaded = fs.read_json(jsonPath);
  if (!loaded.ok) {
    print(loaded.error);
    return 1;
  }
  const record = loaded.value as { label: string };
  print(record.label);

  if (!fs.exists(textPath) || !fs.is_file(textPath) || fs.is_dir(textPath) || !fs.is_dir("./")) {
    print("unexpected file kind");
    return 1;
  }

  const bytes = fs.size(textPath);
  if (!bytes.ok) {
    print(bytes.error);
    return 1;
  }
  print("size " + str(bytes.size));

  const parent = fs.list("./../");
  if (!parent.ok) {
    print(parent.error);
    return 1;
  }
  const listing = fs.list("/flash");
  if (!listing.ok) {
    print(listing.error);
    return 1;
  }
  return 0;
}
