/**
 * pbtxtBridge.ts
 *
 * Parses protobuf text format (.pbtxt) strings into @bufbuild/protobuf typed messages
 * using protobufjs v8's built-in text format parser as the engine.
 *
 * Bridge chain:
 *   pbtxt string
 *     → protobufjs textformat.fromText()  (parse text into protobufjs Message)
 *     → .encode().finish()               (serialize to binary wire format)
 *     → fromBinary(PageSchema, bytes)     (@bufbuild/protobuf typed Page)
 *
 * Requires protobufjs@8.6.3+ for built-in text format support.
 */

import * as protobuf from "protobufjs";
import { fromText, toText } from "protobufjs/ext/textformat";
import { fromBinary } from "@bufbuild/protobuf";
import { PageSchema } from "../gen/ts/layout_pb";

// Load the proto definition once at module init.
// We use protobufjs.parse() (works in browser, no fs needed).
const PROTO_SOURCE = `
syntax = "proto3";

package ui;

message Page {
  string page_id = 1;
  string title = 2;
  repeated Component components = 3;
}

message Component {
  string id = 1;
  string name = 2;
  ComponentType type = 3;
  string text = 4;
  repeated Component children = 5;
  Style style = 6;
  map<string, string> attributes = 7;
  map<string, string> css = 8;
}

enum ComponentType {
  UNKNOWN = 0;
  TEXT = 1;
  BUTTON = 2;
  INPUT = 3;
  CONTAINER = 12;
}

message Style {
  string custom_class = 1;
}
`;

// Parse the proto and get the Page type
const root = protobuf.parse(PROTO_SOURCE).root;
const PageType = root.lookupType("ui.Page");

/**
 * Parse a .pbtxt string into a @bufbuild/protobuf Page message.
 *
 * @param text - The pbtxt string to parse
 * @returns A typed Page message compatible with the renderer
 */
export function parsePbtxtToPage(text: string) {
  // 1. Parse pbtxt into protobufjs Message
  const msg = fromText(PageType, text);

  // 2. Encode to binary wire format
  const bytes = PageType.encode(msg).finish();

  // 3. Decode into @bufbuild/protobuf typed message
  return fromBinary(PageSchema, bytes as Uint8Array);
}
