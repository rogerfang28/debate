import { API_BASE } from '../../lib/config.js';
import { UIEventSchema, EventType } from '../../../../protos/event_pb.js'; // adjust path
import { create, toBinary } from '@bufbuild/protobuf';

export default async function sendDataToBackend() {
  try {
    // 1️⃣ Create a UIEvent message
    const event = create(UIEventSchema, {
      eventId: crypto.randomUUID(), // camelCase property names
      componentId: "btn1",
      type: EventType.CLICK,
      timestamp: Date.now(),
      data: { message: "Hello from frontend!" }
    });

    // 2️⃣ Encode to protobuf binary
    const binary = toBinary(UIEventSchema, event);

    // 3️⃣ Send to backend
    const res = await fetch(`${API_BASE}/events`, { // changed to match /api/events
      method: "POST",
      headers: { "Content-Type": "application/octet-stream" }, // correct for binary
      body: binary
    });

    // 4️⃣ Read server response
    const text = await res.text();
    console.log("Server says:", text);
  } catch (err) {
    console.error("❌ Error sending data:", err);
  }
}
