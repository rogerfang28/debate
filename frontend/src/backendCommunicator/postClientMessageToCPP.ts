// src/functions/postClientMessageToCPP.ts
import { create, toBinary, fromBinary } from "@bufbuild/protobuf";
import {
  ClientMessageSchema,
  PageDataSchema,
  ComponentDataSchema,
} from "../../../src/gen/ts/client_message_pb";
import { PageSchema } from "../../../src/gen/ts/layout_pb";

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
    return page;

  } catch (err) {
    console.error("❌ Error sending ClientMessage to server:", err);
  }
}
