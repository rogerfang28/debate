// // src/functions/sendDataToCPP.ts
// import { create, toBinary } from "@bufbuild/protobuf";
// import {
//   UIEventSchema,
//   EventType,
//   EventValueSchema,
//   EventDataSchema,
// } from "../../../src/gen/ts/event_pb";

// export interface EventData {
//   componentId?: string;
//   eventType?: EventType;
//   actionId?: string | number;
//   eventName?: string;
//   data?: {
//     value?: string;
//     checked?: boolean;
//     type?: string;
//     name?: string;
//     text?: string;
//     componentValue?: string | number | boolean;
//     [key: string]: any;
//   };
//   timestamp?: number;
// }

// /**
//  * Send a UIEvent protobuf to your C++ server (POST / on :8080).
//  * Matches the server’s expectation: Content-Type: application/x-protobuf.
//  */
// export default async function sendDataToCPP(
//   eventData: EventData,
//   opts?: {
//     endpoint?: string;        // override target if needed
//     withCredentials?: boolean // set true only if your C++ CORS allows credentials
//   }
// ): Promise<void> {
//   try {
//     console.log("🚀 sendDataToCPP received:", eventData);
//     console.log("🚀 eventData.data keys:", Object.keys(eventData.data || {}));

//     // 1) Build UIEvent (same structure you had)
//     const event = create(UIEventSchema, {
//       eventId: crypto.randomUUID(),
//       componentId: eventData.componentId || "unknown",
//       type: eventData.eventType || EventType.ACTION,
//       timestamp: BigInt(eventData.timestamp || Date.now()),

//       actionId: String(eventData.actionId || ""),

//       eventData: Object.entries(eventData.data || {}).map(([key, value]) =>
//         create(EventDataSchema, {
//           key,
//           value: create(EventValueSchema, {
//             // keep your simple "textValue" mapping
//             value: { case: "textValue", value: String(value) },
//           }),
//         })
//       ),

//       // legacy map (stringified)
//       data: Object.fromEntries(
//         Object.entries(eventData.data || {}).map(([k, v]) => [k, String(v)])
//       ),
//     });

//     // 2) Encode protobuf
//     const bytes = toBinary(UIEventSchema, event);

//     // 3) POST to C++ server root (default http(s)://<host>:8080/)
//     const endpoint =
//       opts?.endpoint ?? `${location.protocol}//${location.hostname}:8080/`;

//     const res = await fetch(endpoint, {
//       method: "POST",
//       headers: { "Content-Type": "application/x-protobuf" },
//       body: bytes,
//       cache: "no-store",
//       credentials: opts?.withCredentials ? "include" : "same-origin",
//     });

//     if (!res.ok) {
//       const text = await res.text().catch(() => "");
//       throw new Error(`POST failed: ${res.status} ${res.statusText} ${text}`);
//     }

//     // Your C++ server currently responds 204 No Content, so there’s nothing to read.
//     console.log("✅ Event posted to C++ server.");
//   } catch (err) {
//     console.error("❌ Error sending data to C++ server:", err);
//   }
// }
