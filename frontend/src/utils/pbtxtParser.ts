/**
 * pbtxtParser.ts
 *
 * Loads .pbtxt files and parses them into @bufbuild/protobuf typed Page messages.
 * Uses protobufjs v8's built-in text format parser (fromText) as the engine,
 * then bridges to @bufbuild/protobuf types via binary wire format.
 */

import { parsePbtxtToPage } from "./pbtxtBridge";
import type { Page } from "../../../src/gen/ts/layout_pb";

/**
 * Read a .pbtxt file and parse it into a @bufbuild/protobuf Page message.
 * Opens a file picker dialog for the user to select a file.
 */
export function loadPbtxtFromFile(): Promise<Page | null> {
  return new Promise((resolve) => {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = ".pbtxt,.textproto,.txt,.proto";

    input.onchange = async () => {
      const file = input.files?.[0];
      if (!file) { resolve(null); return; }

      try {
        const text = await file.text();
        const page = parsePbtxtToPage(text);
        resolve(page);
      } catch (err) {
        console.error("Failed to parse pbtxt file:", err);
        throw err;
      }
    };

    input.oncancel = () => resolve(null);
    input.click();
  });
}
