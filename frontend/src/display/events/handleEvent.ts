import sendDataToCPP from "../../backendCommunicator/postEventToCPP.ts";
import { EventType } from "../../../../src/gen/js/event_pb.js";
import getEntirePage from "../getEntirePage.js";

// TypeScript interfaces
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

type EventName = 'onClick' | 'onChange' | 'onSubmit';

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
  // collectFrom?: string[]
): Promise<void> {
  try {
    console.log(
      "📤 handleEvent got actionId:",
      actionId,
      "for event:",
      eventName,
      "on component:",
      component.id
      // "collectFrom:",
      // collectFrom
    );

    if (e?.preventDefault && (eventName === "onSubmit" || e.type === "submit")) {
      e.preventDefault();
    }

    // Collect ALL input data from the entire page
    const entirePageData = getEntirePage();
    console.log(`📦 Collected ${Object.keys(entirePageData).length} fields from entire page`);

    await sendDataToCPP({
      componentId: component.id || "unknown",
      eventType: eventTypeMap[eventName as EventName] || EventType.UNKNOWN,
      actionId,
      eventName,
      data: {
        // Include the specific event target data
        value: e?.target?.value,
        checked: e?.target?.checked,
        type: e?.target?.type,
        name: e?.target?.name || component.name,
        text: component.text,
        componentValue: component.value,
        // Include ALL page input data
        ...entirePageData,
      },
      timestamp: Date.now(),
    });

    // 🔹 Instantly refresh the page after sending event
    window.reloadPage?.();

  } catch (error: unknown) {
    console.error(`Error handling event ${eventName}:`, error);
  }
}
