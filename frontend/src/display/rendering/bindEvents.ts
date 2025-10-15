// import handleEvent from "../events/handleEvent";

// // TypeScript interfaces
// export interface ComponentEvent {
//   [eventName: string]: string | number;
// }

// interface ComponentProps {
//   events?: ComponentEvent;
//   [key: string]: any;
// }

// interface BoundEvents {
//   [reactEventName: string]: (e: React.SyntheticEvent) => void;
// }

// /**
//  * Binds backend-defined events to React event handlers
//  * so that they call handleEvent() with the correct actionId.
//  */
// export default function bindEvents(component: ComponentProps): BoundEvents {
//   const boundEvents: BoundEvents = {};

//   // if (!component?.events) return boundEvents; // no events to bind
//   // instead of that, check what type of component it is, like a button or input, and only then bind events
//   // there are no events from the component anymore

//   for (const [eventName, eventConfig] of Object.entries(component.events)) {
//     // Convert backend event names to valid React event prop names
//     let reactEventName: string;

//     if (eventName === "onEnter") {
//       // Special case: map "onEnter" to React's onKeyDown
//       reactEventName = "onKeyDown";
//     } else {
//       // Ensure the event name starts lowercase (React props are camelCase)
//       reactEventName = eventName.charAt(0).toLowerCase() + eventName.slice(1);
//     }

//     boundEvents[reactEventName] = (e: React.SyntheticEvent) => {
//       // Special handling for "onEnter"
//       if (eventName === "onEnter" && (e as React.KeyboardEvent).key !== "Enter") return;

//       // Parse event configuration - it could be just actionId (string/number) or an object with actionId and collectFrom
//       let actionId: string | number;
//       // let collectFrom: string[] | undefined;

//       console.log("🔍 bindEvents raw eventConfig:", eventConfig, typeof eventConfig);
//       console.log("🔍 eventConfig stringified:", JSON.stringify(eventConfig));

//       if (typeof eventConfig === 'string') {
//         // Check if it's a JSON string that needs parsing
//         try {
//           const parsed = JSON.parse(eventConfig);
//           if (typeof parsed === 'object' && parsed.actionId) {
//             actionId = parsed.actionId;
//             // collectFrom = parsed.collectFrom;
//             console.log("🔍 Parsed JSON string - actionId:", actionId);
//           } else {
//             // It's just a regular string actionId
//             actionId = eventConfig;
//             console.log("🔍 Parsed string actionId:", actionId);
//           }
//         } catch {
//           // Not valid JSON, treat as actionId
//           actionId = eventConfig;
//           console.log("🔍 Parsed legacy string format - actionId:", actionId);
//         }
//       } else if (typeof eventConfig === 'object' && eventConfig !== null) {
//         // New format: { actionId: "joinRoom", collectFrom: ["joinRoomForm"] }
//         actionId = (eventConfig as any).actionId;
//         // collectFrom = (eventConfig as any).collectFrom;
//         console.log("🔍 Parsed object format - actionId:", actionId);
//       } else {
//         // Legacy format: just the actionId
//         actionId = eventConfig;
//         console.log("🔍 Parsed legacy format - actionId:", actionId);
//       }

//       console.log("🔧 bindEvents parsed:", { eventName, actionId });
//       // Pass full event info to handleEvent
//       handleEvent(
//         e as any,    // Cast to avoid TypeScript type conflicts
//         component,
//         eventName,   // backend event name ("onClick", "onEnter")
//         actionId    // actionId from the config
//       );
//     };
//   }

//   return boundEvents;
// }
