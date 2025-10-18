// src/functions/postClientMessageToCPP.ts
import { create, toBinary } from "@bufbuild/protobuf";
import {
  ClientMessageSchema,
  PageDataSchema,
  ComponentDataSchema,
} from "../../../src/gen/ts/client_message_pb";

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
 */
export default async function postClientMessageToCPP(
  eventData: EventData,
  opts?: {
    endpoint?: string;        // override target if needed
    withCredentials?: boolean // set true only if your C++ CORS allows credentials
  }
): Promise<void> {
  try {
    console.log("🚀 postClientMessageToCPP received:", eventData);
    console.log("🚀 eventData.data keys:", Object.keys(eventData.data || {}));

    // Build list of ComponentData from the page data
    const components = Object.entries(eventData.data || {}).map(([id, value]) =>
      create(ComponentDataSchema, {
        id,
        value: String(value),
        type: "", // Can enhance this later if needed
      })
    );

    // Build PageData
    const pageData = create(PageDataSchema, {
      pageId: "current-page", // TODO: Get actual page ID
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
      credentials: opts?.withCredentials ? "include" : "same-origin",
    });

    if (!res.ok) {
      const text = await res.text().catch(() => "");
      throw new Error(`POST failed: ${res.status} ${res.statusText} ${text}`);
    }

    console.log("✅ ClientMessage posted to C++ server.");
  } catch (err) {
    console.error("❌ Error sending ClientMessage to C++ server:", err);
  }
}
