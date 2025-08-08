import sendDataToBackend, { EventData } from "./sendData";
import { EventType } from "../../../../src/gen/event_pb.js";

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
  actionId: string | number
): Promise<void> {
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
      eventType: eventTypeMap[eventName as EventName] || EventType.UNKNOWN,
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

  } catch (error: unknown) {
    console.error(`Error handling event ${eventName}:`, error);
  }
}
