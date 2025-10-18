import postClientMessageToCPP from "../../backendCommunicator/postClientMessageToCPP.ts";
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
    console.log('📋 Full page data:', entirePageData);

    await postClientMessageToCPP({
      componentId: component.id || "unknown",
      eventType: eventName,
      actionId,
      eventName,
      data: entirePageData, // Send complete page data
      timestamp: Date.now(),
    });

    // 🔹 Instantly refresh the page after sending event
    window.reloadPage?.();

  } catch (error: unknown) {
    console.error(`Error handling event ${eventName}:`, error);
  }
}
