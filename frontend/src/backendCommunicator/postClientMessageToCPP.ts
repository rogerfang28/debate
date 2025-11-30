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
    [key: string]: any; // All page input data from getEntirePage()
  };
  timestamp?: number;
}

/**
 * Send a ClientMessage protobuf to your C++ server (POST / on :8080).
 * Contains the event info + complete page state with all component values.
 * Returns the Page protobuf response from the server.
 */
export default async function postClientMessageToCPP(
  eventData: EventData,
  opts?: {
    endpoint?: string;        // override target if needed
    withCredentials?: boolean // set true only if your C++ CORS allows credentials
  }
): Promise<any> {
  try {
    console.log("🚀 postClientMessageToCPP received:", eventData);
    console.log("🚀 eventData.data keys:", Object.keys(eventData.data || {}));

    // Extract page ID (stored with special _pageId key)
    const pageId = eventData.data?._pageId as string || "unknown-page";
    
    // Build list of ComponentData from the page data (excluding _pageId)
    const components = Object.entries(eventData.data || {})
      .filter(([id]) => id !== '_pageId') // Exclude the special _pageId field
      .map(([id, value]) =>
        create(ComponentDataSchema, {
          id,
          value: String(value),
          type: "", // Can enhance this later if needed
        })
      );

    // Build PageData
    const pageData = create(PageDataSchema, {
      pageId: pageId,
      components,
    });

    // Build ClientMessage with event + page state
    const clientMessage = create(ClientMessageSchema, {
      componentId: eventData.componentId || "unknown",
      eventType: eventData.eventName || "click",
      pageData,
    });

    console.log("📦 ClientMessage:", {
      componentId: clientMessage.componentId,
      eventType: clientMessage.eventType,
      componentsCount: clientMessage.pageData?.components.length || 0,
    });

    // Encode protobuf
    const bytes = toBinary(ClientMessageSchema, clientMessage);

    // POST to C++ server root (default http(s)://<host>:8080/)
    const endpoint =
      opts?.endpoint ?? `${location.protocol}//${location.hostname}:8080/clientmessage`;

    const res = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/x-protobuf" },
      body: bytes as BodyInit,
      cache: "no-store",
      credentials: "include", // Always include cookies
    });

    if (!res.ok) {
      const text = await res.text().catch(() => "");
      throw new Error(`POST failed: ${res.status} ${res.statusText} ${text}`);
    }

    console.log("✅ ClientMessage posted to C++ server.");
    
    // Decode the Page protobuf response
    const arrayBuffer = await res.arrayBuffer();
    const page_bytes = new Uint8Array(arrayBuffer);
    const page = fromBinary(PageSchema, page_bytes);
    
    console.log("📄 Received Page:", page);
    return page;
    
  } catch (err) {
    console.error("❌ Error sending ClientMessage to C++ server:", err);
  }
}
