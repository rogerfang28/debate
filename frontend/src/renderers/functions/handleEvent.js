import sendDataToBackend from "./sendData.js";
import { EventType } from "../../../../protos/event_pb.js"; // adjust path
import { eventTypeMap } from "./renderPage.jsx"; // import your existing mapping

export default function handleEvent(e, component, eventName, actionId, onReload) {
  try {
    // Prevent form submissions from reloading the page
    if (e?.preventDefault && (eventName === "onSubmit" || e.type === "submit")) {
      e.preventDefault();
    }

    sendDataToBackend({
      componentId: component.id || "unknown",
      eventType: eventTypeMap[eventName] || EventType.UNKNOWN,
      actionId,
      eventName,
      data: {
        value: e?.target?.value,
        checked: e?.target?.checked,
        type: e?.target?.type,
        name: e?.target?.name || component.name,
        text: component.text,
        componentValue: component.value,
      },
      timestamp: Date.now(),
    });

    // Slight delay before reloading
    setTimeout(() => onReload?.(), 100);
  } catch (error) {
    console.error(`Error handling event ${eventName}:`, error);
  }
}
