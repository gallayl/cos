import { describe, expect, it } from "vitest";

import {
  createConsoleTranscript,
  lineVisible,
  normalizeConsoleMessage,
  pushConsoleChunk,
  severityOf,
} from "./console-lines";

describe("console lines", () => {
  it("classifies colored ESP-IDF log lines and leaves command output unfiltered", () => {
    expect(severityOf("\u001b[0;31mE (1200) wifi: failed\u001b[0m")).toBe("error");
    expect(severityOf("W (20) console: careful")).toBe("warn");
    expect(severityOf("I (3) main: ready")).toBe("info");
    expect(severityOf("COS/flash> help")).toBeNull();
  });

  it("hides logs below the selected threshold and always shows other output", () => {
    expect(lineVisible("info", "warn")).toBe(false);
    expect(lineVisible("warn", "warn")).toBe(true);
    expect(lineVisible("error", "error")).toBe(true);
    expect(lineVisible("warn", "error")).toBe(false);
    expect(lineVisible(null, "error")).toBe(true);
  });

  it("splits socket chunks into rows and keeps an unfinished line pending", () => {
    const first = pushConsoleChunk(createConsoleTranscript(), "I (1) main: hi\nCOS>");
    expect(first.lines).toEqual([{ id: 1, text: "I (1) main: hi", severity: "info" }]);
    expect(first.pending).toBe("COS>");

    const second = pushConsoleChunk(first, " help\n");
    expect(second.lines.map((line) => line.text)).toEqual(["I (1) main: hi", "COS> help"]);
    expect(second.pending).toBe("");
  });

  it("sends one trimmed line and drops a blank message", () => {
    expect(normalizeConsoleMessage("  help\n")).toBe("help");
    expect(normalizeConsoleMessage(" \n")).toBeNull();
    expect(normalizeConsoleMessage("x".repeat(200))).toHaveLength(126);
  });
});
