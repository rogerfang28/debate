// src/functions/postClientMessageToCPP.ts
import { create, toBinary, fromBinary } from "@bufbuild/protobuf";
import {
  ClientMessageSchema,
  PageDataSchema,
  ComponentDataSchema,
} from "../../../src/gen/ts/client_message_pb";
import { PageSchema } from "../../../src/gen/ts/layout_pb";
import type { Page } from "../../../src/gen/ts/layout_pb";

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

export default async function sendClientMessageToCPP(
  eventData: EventData
): Promise<Page | null> {
  try {
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
    const endpoint = `${location.protocol}//${location.hostname}:5000`;

    const res = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/x-protobuf" },
      body: bytes as BodyInit,
      cache: "no-store",
      credentials: "include", // Always include cookies
    });

    if (!res.ok) {
      console.error(`❌ Server returned status ${res.status}`);
      return null;
    }

    // Parse the protobuf Page response
    const responseBytes = new Uint8Array(await res.arrayBuffer());
    const page = fromBinary(PageSchema, responseBytes);

    console.log("✅ Received Page from server:", {
      pageId: page.pageId,
      title: page.title,
      componentsCount: page.components.length,
    });

    return page;
    
    // // Check if this was a sign-in action that set a cookie
    // if (clientMessage.pageData?.pageId === "enter_username" && res.status === 200) {
    //   console.log("🔐 Sign-in successful, reloading page...");
    //   // Reload to get the authenticated page
    //   window.location.reload();
    // }
    
    // // Check if this was a logout action
    // if (eventData.componentId === "logoutButton" && res.status === 200) {
    //   console.log("🚪 Logout successful, reloading page...");
    //   // Reload to show sign-in page
    //   window.location.reload();
    // }
  } catch (err) {
    console.error("❌ Error sending ClientMessage to C++ server:", err);
    return null;
  }
}
