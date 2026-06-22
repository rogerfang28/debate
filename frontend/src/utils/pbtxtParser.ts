/**
 * pbtxtParser.ts
 *
 * Lightweight protobuf text format (.pbtxt) parser.
 * Parses pbtxt text into plain JS objects compatible with the layout_pb Page/Component types.
 *
 * Supports:
 *   - Nested messages
 *   - Repeated fields (become arrays)
 *   - Scalar fields (string, number, boolean)
 *   - Maps (map<string, string>)
 *   - Enum values (by name or number)
 *   - Comments (// and /* *)
 */

// ── Enum name → value maps ──────────────────────────────────────────────

const COMPONENT_TYPE_MAP: Record<string, number> = {
  UNKNOWN: 0,
  TEXT: 1,
  BUTTON: 2,
  INPUT: 3,
  CONTAINER: 12,
};

const LAYOUT_TYPE_MAP: Record<string, number> = {
  ABSLT: 0,
  FLEX_ROW: 1,
  FLEX_COL: 2,
  GRID: 3,
  INLINE: 4,
};

// ── Snake-case → camelCase field name mapping ───────────────────────────

const FIELD_MAP: Record<string, string> = {
  page_id: "pageId",
  custom_class: "customClass",
};

function toCamelCase(snake: string): string {
  return FIELD_MAP[snake] ?? snake.replace(/_([a-z])/g, (_, c) => c.toUpperCase());
}

// ── Tokenizer ───────────────────────────────────────────────────────────

type Token =
  | { type: "string"; value: string }
  | { type: "number"; value: number }
  | { type: "ident"; value: string }
  | { type: "symbol"; value: string }
  | { type: "angle"; value: string };

function tokenize(input: string): Token[] {
  const tokens: Token[] = [];
  let i = 0;

  while (i < input.length) {
    const ch = input[i];

    // Skip whitespace
    if (/\s/.test(ch)) { i++; continue; }

    // Skip line comments
    if (ch === "/" && input[i + 1] === "/") {
      while (i < input.length && input[i] !== "\n") i++;
      continue;
    }

    // Skip block comments
    if (ch === "/" && input[i + 1] === "*") {
      i += 2;
      while (i < input.length && !(input[i] === "*" && input[i + 1] === "/")) i++;
      i += 2;
      continue;
    }

    // Quoted strings
    if (ch === '"' || ch === "'") {
      const quote = ch;
      i++;
      let str = "";
      while (i < input.length && input[i] !== quote) {
        if (input[i] === "\\") {
          i++;
          const esc = input[i];
          if (esc === "n") str += "\n";
          else if (esc === "t") str += "\t";
          else if (esc === "\\") str += "\\";
          else if (esc === quote) str += quote;
          else str += esc;
        } else {
          str += input[i];
        }
        i++;
      }
      i++; // skip closing quote
      tokens.push({ type: "string", value: str });
      continue;
    }

    // Numbers (including negative, float, hex)
    if (/[0-9-]/.test(ch) && (ch !== "-" || /[0-9]/.test(input[i + 1] ?? ""))) {
      let num = "";
      if (ch === "-") { num += ch; i++; }
      while (i < input.length && /[0-9.xXa-fA-FeE+-]/.test(input[i])) {
        num += input[i]; i++;
      }
      tokens.push({ type: "number", value: Number(num) });
      continue;
    }

    // Identifiers / keywords
    if (/[a-zA-Z_]/.test(ch)) {
      let ident = "";
      while (i < input.length && /[a-zA-Z0-9_.]/.test(input[i])) {
        ident += input[i]; i++;
      }
      tokens.push({ type: "ident", value: ident });
      continue;
    }

    // Symbols: { } : < >
    if ("{}:<>,;".includes(ch)) {
      tokens.push({ type: ch === "<" || ch === ">" ? "angle" : "symbol", value: ch });
      i++;
      continue;
    }

    // Skip unknown characters
    i++;
  }

  return tokens;
}

// ── Parser ──────────────────────────────────────────────────────────────

interface ParseContext {
  tokens: Token[];
  pos: number;
}

function peek(ctx: ParseContext): Token | undefined {
  return ctx.tokens[ctx.pos];
}

function advance(ctx: ParseContext): Token {
  return ctx.tokens[ctx.pos++];
}

function expect(ctx: ParseContext, type: string, value?: string): Token {
  const tok = advance(ctx);
  if (!tok || tok.type !== type || (value !== undefined && tok.value !== value)) {
    const expected = value ? type + ' "' + value + '"' : type;
    const got = tok ? tok.type + ' "' + tok.value + '"' : "nothing";
    throw new Error("Expected " + expected + " at token " + ctx.pos + ", got " + got);
  }
  return tok;
}

/**
 * Parse a single value from the token stream.
 */
function parseValue(ctx: ParseContext): unknown {
  const tok = peek(ctx);
  if (!tok) throw new Error("Unexpected end of input");

  if (tok.type === "string") {
    advance(ctx);
    return tok.value;
  }

  if (tok.type === "number") {
    advance(ctx);
    return tok.value;
  }

  if (tok.type === "ident" && (tok.value === "true" || tok.value === "false")) {
    advance(ctx);
    return tok.value === "true";
  }

  if (tok.type === "symbol" && tok.value === "{") {
    return parseMessage(ctx);
  }

  if (tok.type === "ident") {
    advance(ctx);
    return tok.value;
  }

  throw new Error("Unexpected token: " + tok.type + ' "' + tok.value + '"');
}

/**
 * Parse a message body (after the opening {).
 * Returns a plain JS object with camelCase keys.
 */
function parseMessage(ctx: ParseContext): Record<string, unknown> {
  expect(ctx, "symbol", "{");
  const obj: Record<string, unknown> = {};

  while (true) {
    const tok = peek(ctx);
    if (!tok || (tok.type === "symbol" && tok.value === "}")) {
      advance(ctx);
      break;
    }

    // Field name
    let fieldName: string;
    if (tok.type === "ident") {
      advance(ctx);
      fieldName = toCamelCase(tok.value);
    } else if (tok.type === "symbol" && tok.value === "[") {
      // Proto extension — skip
      advance(ctx);
      while (peek(ctx) && !(peek(ctx)!.type === "symbol" && peek(ctx)!.value === "]")) {
        advance(ctx);
      }
      expect(ctx, "symbol", "]");
      expect(ctx, "symbol", ":");
      parseValue(ctx);
      continue;
    } else {
      throw new Error("Expected field name, got " + tok.type + ' "' + tok.value + '"');
    }

    const nextTok = peek(ctx);

    // key: value
    if (nextTok && nextTok.type === "symbol" && nextTok.value === ":") {
      advance(ctx);
      const value = parseValue(ctx);

      if (fieldName in obj) {
        const existing = obj[fieldName];
        if (Array.isArray(existing)) {
          existing.push(value);
        } else {
          obj[fieldName] = [existing, value];
        }
      } else {
        obj[fieldName] = value;
      }
    } else if (nextTok && nextTok.type === "symbol" && nextTok.value === "{") {
      // Could be nested message or map — peek ahead
      const savedPos = ctx.pos;
      advance(ctx); // consume {
      const innerTok = peek(ctx);

      if (innerTok && innerTok.type === "ident" && innerTok.value === "key") {
        // It's a map<string, string>
        const mapObj: Record<string, string> = {};
        while (peek(ctx) && !(peek(ctx)!.type === "symbol" && peek(ctx)!.value === "}")) {
          const keyTok = expect(ctx, "ident");
          expect(ctx, "symbol", ":");
          const valTok = expect(ctx, "string");
          mapObj[keyTok.value] = valTok.value;

          // Handle "value" key if present (map entry format: key: "k" value: "v")
          const maybeValue = peek(ctx);
          if (maybeValue && maybeValue.type === "ident" && maybeValue.value === "value") {
            advance(ctx);
            expect(ctx, "symbol", ":");
            const realVal = expect(ctx, "string");
            mapObj[keyTok.value] = realVal.value;
          }
        }
        expect(ctx, "symbol", "}");
        obj[fieldName] = mapObj;
      } else {
        // Nested message — restore and parse
        ctx.pos = savedPos;
        const nestedMsg = parseMessage(ctx);
        if (fieldName in obj) {
          const existing = obj[fieldName];
          if (Array.isArray(existing)) {
            existing.push(nestedMsg);
          } else {
            obj[fieldName] = [existing, nestedMsg];
          }
        } else {
          obj[fieldName] = nestedMsg;
        }
      }
    }
  }

  return obj;
}

// ── Post-processing ─────────────────────────────────────────────────────

/**
 * Convert enum name strings to their numeric values.
 */
function resolveEnums(obj: Record<string, unknown>): void {
  if (typeof obj["type"] === "string") {
    const upper = (obj["type"] as string).toUpperCase();
    if (upper in COMPONENT_TYPE_MAP) {
      obj["type"] = COMPONENT_TYPE_MAP[upper];
    }
  }
  if (Array.isArray(obj["children"])) {
    for (const child of obj["children"]) {
      if (typeof child === "object" && child !== null) {
        resolveEnums(child as Record<string, unknown>);
      }
    }
  }
  if (Array.isArray(obj["components"])) {
    for (const comp of obj["components"]) {
      if (typeof comp === "object" && comp !== null) {
        resolveEnums(comp as Record<string, unknown>);
      }
    }
  }
}

/**
 * Parse top-level pbtxt content (no outer braces).
 * Handles field: value and field { ... } at the top level.
 */
function parseTopLevel(ctx: ParseContext): Record<string, unknown> {
  const obj: Record<string, unknown> = {};

  while (true) {
    const tok = peek(ctx);
    if (!tok) break;

    // Field name
    let fieldName: string;
    if (tok.type === "ident") {
      advance(ctx);
      fieldName = toCamelCase(tok.value);
    } else {
      // Skip unexpected tokens
      advance(ctx);
      continue;
    }

    const nextTok = peek(ctx);

    // key: value
    if (nextTok && nextTok.type === "symbol" && nextTok.value === ":") {
      advance(ctx);
      const value = parseValue(ctx);

      if (fieldName in obj) {
        const existing = obj[fieldName];
        if (Array.isArray(existing)) {
          existing.push(value);
        } else {
          obj[fieldName] = [existing, value];
        }
      } else {
        obj[fieldName] = value;
      }
    } else if (nextTok && nextTok.type === "symbol" && nextTok.value === "{") {
      // Nested message
      const nestedMsg = parseMessage(ctx);
      if (fieldName in obj) {
        const existing = obj[fieldName];
        if (Array.isArray(existing)) {
          existing.push(nestedMsg);
        } else {
          obj[fieldName] = [existing, nestedMsg];
        }
      } else {
        obj[fieldName] = nestedMsg;
      }
    } else {
      // Malformed — skip
      continue;
    }
  }

  return obj;
}

// ── Public API ──────────────────────────────────────────────────────────

/**
 * Parse a .pbtxt string into a Page object compatible with the layout renderer.
 */
export function parsePbtxtToPage(text: string): Record<string, unknown> {
  const tokens = tokenize(text);
  const ctx: ParseContext = { tokens, pos: 0 };
  const raw = parseTopLevel(ctx);
  resolveEnums(raw);
  return raw;
}

/**
 * Read a .pbtxt file and parse it into a Page object.
 * Opens a file picker dialog for the user to select a file.
 */
export function loadPbtxtFromFile(): Promise<Record<string, unknown> | null> {
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
