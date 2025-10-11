// take in an event object and send it to the backend
import { create, toBinary } from "@bufbuild/protobuf";
import { EventDataSchema } from "../../../src/gen/ts/eventData_pb";

/**
 * Sends an EventData protobuf message to the C++ backend.
 *
 * @param actionId - unique string identifying the event/action
 * @param data - optional key/value metadata for the event
 * @param opts - optional overrides (endpoint, credentials)
 */
export default async function sendEventDataToBackend(
  actionId: string,
  data: Record<string, string> = {},
  opts?: {
    endpoint?: string;
    withCredentials?: boolean;
  }
): Promise<void> {
  try {
    console.log("🚀 sendEventDataToCPP:", { actionId, data });

    // 1️⃣ Build EventData protobuf
    const event = create(EventDataSchema, {
      actionId,
      data,
    });

    // 2️⃣ Encode to binary protobuf
    const bytes = toBinary(EventDataSchema, event);

    // 3️⃣ Determine endpoint (default to same host, port 8080)
    const endpoint =
      opts?.endpoint ?? `${location.protocol}//${location.hostname}:8080/eventData`;

    // 4️⃣ Send POST request to C++ backend
    const res = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/x-protobuf" },
      body: bytes,
      cache: "no-store",
      credentials: opts?.withCredentials ? "include" : "same-origin",
    });

    if (!res.ok) {
      const text = await res.text().catch(() => "");
      throw new Error(`POST failed: ${res.status} ${res.statusText} ${text}`);
    }

    console.log("✅ EventData sent successfully.");
  } catch (err) {
    console.error("❌ Error sending EventData to C++:", err);
  }
}
