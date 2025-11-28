// // src/functions/sendClientMessageGrpc.ts
// import { create } from "@bufbuild/protobuf";
// import {
//   ClientMessageSchema,
//   PageDataSchema,
//   ComponentDataSchema,
// } from "../../../src/gen/ts/client_message_pb";

// // generated service descriptor from protoc-gen-es
// import { ClientMessageService } from "../../../src/gen/ts/ClientMessageService_pb";

// import { createPromiseClient } from "@connectrpc/connect";
// import { createGrpcWebTransport } from "@connectrpc/connect-web";

// export interface EventData {
//   componentId?: string;
//   eventType?: string;
//   actionId?: string | number;
//   eventName?: string;
//   data?: {
//     [key: string]: any; // All page input data from getEntirePage()
//   };
//   timestamp?: number;
// }

// /**
//  * Send a ClientMessage to your C++ gRPC server using Connect/Grpc-Web.
//  * Contains the event info + complete page state with all component values.
//  */
// export default async function sendClientMessageGrpc(
//   eventData: EventData,
//   opts?: {
//     /** Base URL of your grpc-web / Connect proxy, not the raw C++ server. */
//     endpoint?: string;
//   }
// ): Promise<void> {
//   try {
//     console.log("🚀 sendClientMessageGrpc received:", eventData);
//     console.log("🚀 eventData.data keys:", Object.keys(eventData.data || {}));

//     const pageId = (eventData.data?._pageId as string) || "unknown-page";

//     const components = Object.entries(eventData.data || {})
//       .filter(([id]) => id !== "_pageId")
//       .map(([id, value]) =>
//         create(ComponentDataSchema, {
//           id,
//           value: String(value),
//           type: "",
//         })
//       );

//     const pageData = create(PageDataSchema, {
//       pageId,
//       components,
//     });

//     const clientMessage = create(ClientMessageSchema, {
//       componentId: eventData.componentId || "unknown",
//       eventType: eventData.eventName || "click",
//       pageData,
//     });

//     console.log("📦 ClientMessage (gRPC):", {
//       componentId: clientMessage.componentId,
//       eventType: clientMessage.eventType,
//       componentsCount: clientMessage.pageData?.components.length || 0,
//     });

//     // Where the browser talks to grpc-web / Connect proxy
//     const baseUrl =
//       opts?.endpoint ??
//       `${location.protocol}//${location.hostname}:8080`; // example proxy port

//     const transport = createGrpcWebTransport({
//       baseUrl,
//     });

//     const client = createPromiseClient(ClientMessageService, transport);

//     // 🔴 This is the actual gRPC call
//     const pageResponse = await client.sendClientMessage(clientMessage);

//     console.log("✅ gRPC SendClientMessage response Page:", pageResponse);

//     // Keep your reload logic based on pageId / componentId
//     if (clientMessage.pageData?.pageId === "enter_username") {
//       console.log("🔐 Sign-in flow via gRPC, reloading page...");
//       window.location.reload();
//     }

//     if (eventData.componentId === "logoutButton") {
//       console.log("🚪 Logout via gRPC, reloading page...");
//       window.location.reload();
//     }
//   } catch (err) {
//     console.error("❌ Error sending ClientMessage over gRPC:", err);
//   }
// }
