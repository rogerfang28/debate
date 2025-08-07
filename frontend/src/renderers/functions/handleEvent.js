import sendDataToBackend from "./sendData.js";
import { EventType } from "../../../../src/gen/event_pb.js";

const eventTypeMap = {
  onClick: EventType.CLICK,
  onChange: EventType.INPUT_CHANGE,
  onSubmit: EventType.FORM_SUBMIT,
};

export default async function handleEvent(e, component, eventName, actionId) {
  try {
    console.log(
      "📤 handleEvent got actionId:",
      actionId,
      "for event:",
      eventName,
      "on component:",
      component.id
    );

    if (e?.preventDefault && (eventName === "onSubmit" || e.type === "submit")) {
      e.preventDefault();
    }

    await sendDataToBackend({
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

    // 🔹 Instantly refresh the page after sending event
    window.reloadPage?.();

  } catch (error) {
    console.error(`Error handling event ${eventName}:`, error);
  }
}
