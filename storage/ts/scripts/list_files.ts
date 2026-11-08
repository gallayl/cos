import fs from "@cos/fs";

const path = "/flash";
const result = fs.list(path);

if (result.ok) {
  const entries = result.entries;
  let index = 0;
  while (index < result.count) {
    log(entries[index].name);
    index += 1;
  }
} else {
  log("Could not list " + path + ": " + result.error);
}
