import { API_BASE } from '../../../unused/lib/config.js';
import { UIEventSchema, EventType } from '../../../../../src/gen/event_pb.js';
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
    // 1️⃣ Create a UIEvent message with the new flexible structure
    const event = create(UIEventSchema, {
      eventId: crypto.randomUUID(),
      componentId: eventData.componentId || "unknown",
      type: eventData.eventType || EventType.ACTION, // Use ACTION type for new events
      timestamp: BigInt(eventData.timestamp || Date.now()),
      
      // ✅ Use the new action_id field
      actionId: String(eventData.actionId || ""),
      
      // ✅ Convert data to the new flexible event_data array
      eventData: Object.entries(eventData.data || {}).map(([key, value]) => ({
        key,
        value: convertToEventValue(value)
      })),
      
      // Keep legacy data for backward compatibility if needed
      data: {
        ...eventData.data,
        actionId: eventData.actionId || ""
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

// Helper function to convert JavaScript values to EventValue protobuf format
function convertToEventValue(value: any) {
  if (typeof value === 'string') {
    return { textValue: value };
  } else if (typeof value === 'number') {
    if (Number.isInteger(value)) {
      return { numberValue: BigInt(value) };
    } else {
      return { decimalValue: value };
    }
  } else if (typeof value === 'boolean') {
    return { booleanValue: value };
  } else if (Array.isArray(value) && value.every(v => typeof v === 'string')) {
    return { listValue: { values: value } };
  } else {
    // Fallback to string representation
    return { textValue: String(value) };
  }
}
