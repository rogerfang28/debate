import { API_BASE } from '../unused/lib/config.js'; // ! PROBLEM?
import { UIEventSchema, EventType, EventValueSchema, EventDataSchema } from '../../../src/gen/ts/event_pb.ts';
import { create, toBinary } from '@bufbuild/protobuf';
import { testCPPBackend } from './testCPPBackend.ts';
import { testPing } from './testPing.ts';
import sendDataToCPP from './postEventToCPP.ts';

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
  sendDataToCPP(eventData);
  // testPing();
  try {
    console.log("🚀 sendDataToBackend received:", eventData);
    console.log("🚀 eventData.data keys:", Object.keys(eventData.data || {}));
    
    // 1️⃣ Create a UIEvent message with the new flexible structure
    const event = create(UIEventSchema, {
      eventId: crypto.randomUUID(),
      componentId: eventData.componentId || "unknown",
      type: eventData.eventType || EventType.ACTION, // Use ACTION type for new events
      timestamp: BigInt(eventData.timestamp || Date.now()),
      
      // ✅ Use the new action_id field
      actionId: String(eventData.actionId || ""),
      
      // ✅ Convert data to the new flexible event_data array
      eventData: Object.entries(eventData.data || {}).map(([key, value]) => {
        console.log(`🔄 Converting ${key}:`, value);
        return create(EventDataSchema, { 
          key, 
          value: create(EventValueSchema, {
            value: {
              value: String(value),
              case: "textValue"
            }
          })
        });
      }),
      
      // Keep legacy data for backward compatibility if needed (convert all to strings)
      data: Object.fromEntries(
        Object.entries(eventData.data || {}).map(([key, value]) => [key, String(value)])
      )
    });

    // 2️⃣ Encode to protobuf binary
    const binary = toBinary(UIEventSchema, event);

    // 3️⃣ Send to backend WITH credentials (cookies)
    const res = await fetch(`${API_BASE}/events`, {
      method: "POST",
      headers: { "Content-Type": "application/octet-stream" },
      body: new Uint8Array(binary),
      credentials: "include" // 🔹 send cookies/session ID
    });

    console.log("Server says:", await res.text());
  } catch (err: unknown) {
    console.error("❌ Error sending data:", err);
  }
}
