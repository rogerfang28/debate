import { API_BASE } from '../../lib/config.js';
import { UIEventSchema, EventType } from '../../../../src/gen/event_pb.js';
import { create, toBinary } from '@bufbuild/protobuf';

// TypeScript interface for event data
export interface EventData {
  componentId?: string;
  eventType?: EventType;
  actionId?: string | number;
  eventName?: string;
  data?: {
    value?: string;
    checked?: boolean;
    type?: string;
    name?: string;
    text?: string;
    componentValue?: string | number | boolean;
    [key: string]: any;
  };
  timestamp?: number;
}

export default async function sendDataToBackend(eventData: EventData): Promise<void> {
  try {
    // 1️⃣ Create a UIEvent message with real event data
    const event = create(UIEventSchema, {
      eventId: crypto.randomUUID(),
      componentId: eventData.componentId || "unknown",
      type: eventData.eventType || EventType.UNKNOWN,
      timestamp: eventData.timestamp || Date.now(),
      data: {
        ...eventData.data,
        actionId: eventData.actionId || "" // inside `data` since proto has no actionId field
      }
    });

    // 2️⃣ Encode to protobuf binary
    const binary = toBinary(UIEventSchema, event);

    // 3️⃣ Send to backend WITH credentials (cookies)
    const res = await fetch(`${API_BASE}/events`, {
      method: "POST",
      headers: { "Content-Type": "application/octet-stream" },
      body: binary,
      credentials: "include" // 🔹 send cookies/session ID
    });

    console.log("Server says:", await res.text());
  } catch (err: unknown) {
    console.error("❌ Error sending data:", err);
  }
}
