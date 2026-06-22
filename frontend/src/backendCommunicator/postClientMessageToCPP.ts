// src/functions/postClientMessageToCPP.ts
import { create, toBinary, fromBinary } from "@bufbuild/protobuf";
import {
  ClientMessageSchema,
  PageDataSchema,
  ComponentDataSchema,
} from "../../../src/gen/ts/client_message_pb";
import { PageSchema, ComponentSchema, StyleSchema } from "../../../src/gen/ts/layout_pb";
import { parsePbtxtToPage } from "../utils/pbtxtParser";

export interface EventData {
  componentId?: string;
  eventType?: string;
  actionId?: string | number;
  eventName?: string;
  data?: {
    [key: string]: any;
  };
  timestamp?: number;
}

// Resolve the API base URL:
// 1. VITE_API_URL env var (set at build time for production)
// 2. Empty string (use relative paths, relies on Vite proxy in dev)
const API_BASE = import.meta.env.VITE_API_URL?.trim() || "";

/**
 * Serialize a Page protobuf message to pbtxt text format.
 * This converts the binary protobuf response to pbtxt so it can be
 * rendered through the pbtxt parser path for debugging/comparison.
 */
function pageToPbtxt(page: any): string {
  const lines: string[] = [];

  if (page.pageId) {
    lines.push(`page_id: "${page.pageId}"`);
  }
  if (page.title) {
    lines.push(`title: "${page.title}"`);
  }
  if (page.components && Array.isArray(page.components)) {
    for (const comp of page.components) {
      lines.push("");
      serializeComponent(comp, lines, "components");
    }
  }

  return lines.join("\n");
}

function serializeComponent(comp: any, lines: string[], fieldName: string, indent: string = ""): void {
  lines.push(`${indent}${fieldName} {`);

  if (comp.id) {
    lines.push(`${indent}  id: "${comp.id}"`);
  }
  if (comp.name) {
    lines.push(`${indent}  name: "${comp.name}"`);
  }
  if (comp.type !== undefined && comp.type !== 0) {
    const typeNames: Record<number, string> = {
      0: "UNKNOWN", 1: "TEXT", 2: "BUTTON", 3: "INPUT", 12: "CONTAINER"
    };
    lines.push(`${indent}  type: ${typeNames[comp.type] || "UNKNOWN"}`);
  }
  if (comp.text) {
    const escaped = comp.text.replace(/\\/g, "\\\\").replace(/"/g, '\\"').replace(/\n/g, "\\n");
    lines.push(`${indent}  text: "${escaped}"`);
  }

  // Style
  if (comp.style?.customClass) {
    lines.push(`${indent}  style {`);
    lines.push(`${indent}    custom_class: "${comp.style.customClass}"`);
    lines.push(`${indent}  }`);
  }

  // CSS map
  if (comp.css && typeof comp.css === "object" && Object.keys(comp.css).length > 0) {
    for (const [key, value] of Object.entries(comp.css)) {
      lines.push(`${indent}  css { key: "${key}" value: "${value}" }`);
    }
  }

  // Children (nested components)
  if (comp.children && Array.isArray(comp.children)) {
    for (const child of comp.children) {
      serializeComponent(child, lines, "children", indent + "  ");
    }
  }

  lines.push(`${indent}}`);
}

/**
 * Send a ClientMessage protobuf to the C++ server.
 * In dev: uses Vite proxy (/api/clientmessage → localhost:8080/clientmessage)
 * In prod: uses VITE_API_URL directly (e.g., https://backend.onrender.com/clientmessage)
 */
export default async function postClientMessageToCPP(
  eventData: EventData,
  opts?: {
    endpoint?: string;
    withCredentials?: boolean;
  }
): Promise<any> {
  try {
    console.log("🚀 postClientMessageToCPP received:", eventData);
    console.log("🚀 eventData.data keys:", Object.keys(eventData.data || {}));

    const pageId = eventData.data?._pageId as string || "login";

    const components = Object.entries(eventData.data || {})
      .filter(([id]) => id !== '_pageId')
      .map(([id, value]) =>
        create(ComponentDataSchema, {
          id,
          value: String(value),
          type: "",
        })
      );

    const pageData = create(PageDataSchema, {
      pageId,
      components,
    });

    const clientMessage = create(ClientMessageSchema, {
      componentId: eventData.componentId ?? "",
      eventType: eventData.eventType ?? eventData.eventName ?? "",
      pageData,
    });

    console.log("📦 ClientMessage:", {
      componentId: clientMessage.componentId,
      eventType: clientMessage.eventType,
      componentsCount: clientMessage.pageData?.components.length || 0,
    });

    const bytes = toBinary(ClientMessageSchema, clientMessage);

    // Build the full endpoint URL
    const relativePath = opts?.endpoint ?? "/api/clientmessage";
    // In production, prepend the API base URL; in dev, use relative path for proxy
    const endpoint = API_BASE
      ? `${API_BASE.replace(/\/+$/, '')}${relativePath.replace(/^\/api/, '')}`
      : relativePath;

    console.log("📡 Sending to:", endpoint);

    const res = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/x-protobuf" },
      body: bytes as BodyInit,
      cache: "no-store",
      credentials: opts?.withCredentials === false ? "omit" : "include",
    });

    if (!res.ok) {
      const text = await res.text().catch(() => "");
      throw new Error(`POST failed: ${res.status} ${res.statusText} ${text}`);
    }

    console.log("✅ ClientMessage posted to server.");

    const arrayBuffer = await res.arrayBuffer();
    const page_bytes = new Uint8Array(arrayBuffer);
    const page = fromBinary(PageSchema, page_bytes);

    console.log("📄 Received Page:", page);

    // Also convert to pbtxt and render through the pbtxt parser for comparison
    const pbtxt = pageToPbtxt(page);
    console.log("📝 PBTXT output:\n", pbtxt);

    const pbtxtPage = parsePbtxtToPage(pbtxt);
    console.log("🔍 Parsed pbtxt page:", pbtxtPage);

    return pbtxtPage;

  } catch (err) {
    console.error("❌ Error sending ClientMessage to server:", err);
  }
}
