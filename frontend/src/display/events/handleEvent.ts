import sendDataToCPP from "../../backendCommunicator/postEventToCPP.ts";
import sendEventDataToBackend from "../../backendCommunicator/sendEventDataToBackend.ts";
import { EventType } from "../../../../src/gen/js/event_pb.js";
import getInfoFromPage from "../../utils/getInfoFromPage.js";

interface ComponentProps {
  id?: string;
  name?: string;
  text?: string;
  value?: string | number | boolean;
  [key: string]: any;
}

interface EventTarget {
  value?: string;
  checked?: boolean;
  type?: string;
  name?: string;
}

interface CustomEvent {
  preventDefault?: () => void;
  type?: string;
  target?: EventTarget;
}

type EventName = "onClick" | "onChange" | "onSubmit";

const eventTypeMap: Record<EventName, EventType> = {
  onClick: EventType.CLICK,
  onChange: EventType.INPUT_CHANGE,
  onSubmit: EventType.FORM_SUBMIT,
};

export default async function handleEvent(
  e: CustomEvent | null,
  component: ComponentProps,
  eventName: EventName | string,
  actionId: string | number,
  collectFrom?: string[]
): Promise<void> {
  try {
    console.log("📤 handleEvent got actionId:", actionId);

    // Prevent default form submission if needed
    if (e?.preventDefault && (eventName === "onSubmit" || e.type === "submit")) {
      e.preventDefault();
    }

    // 1️⃣ Build data payload (optional)
    const data: Record<string, string> = {};

    if (collectFrom && collectFrom.length > 0) {
      for (const componentId of collectFrom) {
        const value = getInfoFromPage(componentId);
        data[componentId] = String(value ?? "");
        console.log(`📋 Collected from ${componentId}:`, value);
      }
    }

    // 2️⃣ Send simplified EventData message to backend
    await sendEventDataToBackend(String(actionId), data);

    // 3️⃣ Send full UIEvent (legacy) if you still want to support it
    // await sendDataToCPP({
    //   componentId: component.id || "unknown",
    //   eventType: eventTypeMap[eventName as EventName] || EventType.UNKNOWN,
    //   actionId,
    //   eventName,
    //   data: {
    //     value: e?.target?.value,
    //     checked: e?.target?.checked,
    //     type: e?.target?.type,
    //     name: e?.target?.name || component.name,
    //     text: component.text,
    //     componentValue: component.value,
    //   },
    //   timestamp: Date.now(),
    // });

    // 4️⃣ Optionally reload page after event
    window.reloadPage?.();
  } catch (error: unknown) {
    console.error(`❌ Error handling event ${eventName}:`, error);
  }
}
