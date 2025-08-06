import { API_BASE } from '../../lib/config.js';
import { UIEventSchema, EventType } from '../../../../protos/unused/event_pb.js';
import { create, toBinary } from '@bufbuild/protobuf';

export default async function sendDataToBackend(eventData) {
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
  } catch (err) {
    console.error("❌ Error sending data:", err);
  }
}
