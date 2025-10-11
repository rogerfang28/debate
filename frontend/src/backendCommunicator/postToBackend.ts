import { create, toBinary } from "@bufbuild/protobuf";
import { ClientMessageSchema } from "../../../src/gen/ts/clientMessage_pb";
import { EventDataSchema } from "../../../src/gen/ts/eventData_pb";
import { PageSchema } from "../../../src/gen/ts/page_pb";

/**
 * Send both the current Page protobuf and EventData protobuf to the backend.
 *
 * The backend endpoint should expect a binary-encoded `clientMessage.ClientMessage`
 * message (containing { event, page }).
 *
 * @param actionId - The action identifier for the event (e.g. "submitTopic")
 * @param page - A ui.Page protobuf instance representing the current page
 * @param data - Optional key/value map with additional event info
 * @param opts - Optional overrides (endpoint, credentials)
 */
export default async function postToBackend(
  actionId: string,
  page: InstanceType<typeof PageSchema>,
  data: Record<string, string> = {},
  opts?: {
    endpoint?: string;
    withCredentials?: boolean;
  }
): Promise<void> {
  try {
    console.log("🚀 postToBackend sending:", { actionId, data, page });

    // 1️⃣ Build the EventData protobuf
    const event = create(EventDataSchema, {
      actionId,
      data,
    });

    // 2️⃣ Wrap both the event + page into one ClientMessage
    const msg = create(ClientMessageSchema, {
      event,
      page,
    });

    // 3️⃣ Serialize the wrapper message to binary protobuf
    const bytes = toBinary(ClientMessageSchema, msg);

    // 4️⃣ Determine backend endpoint (default → /eventData)
    const endpoint =
      opts?.endpoint ?? `${location.protocol}//${location.hostname}:8080/eventData`;

    // 5️⃣ Send POST request to C++ backend
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

    console.log("✅ postToBackend: Event + Page successfully sent to backend.");
  } catch (err) {
    console.error("❌ postToBackend error:", err);
  }
}
