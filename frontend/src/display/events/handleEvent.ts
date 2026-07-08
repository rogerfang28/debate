import postClientMessageToCPP from "../../backendCommunicator/postClientMessageToCPP.ts";
import getEntirePage from "../getEntirePage.ts";

// Google sign-in callback — set by GoogleLoginComponent
// eslint-disable-next-line @typescript-eslint/no-explicit-any
(window as any).googleLoginCallback = async (credential: string) => {
  console.log("[GoogleAuth] Got ID token from Google, sending to backend...");
  try {
    await postClientMessageToCPP({
      componentId: "googleLoginButton",
      eventType: "onGoogleLogin",
      actionId: "google",
      eventName: "onGoogleLogin",
      data: { _pageId: "login", google_id_token: credential },
      timestamp: Date.now(),
    });
    window.reloadPage?.();
  } catch (error) {
    console.error("[GoogleAuth] Failed to send Google token:", error);
  }
};

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
      "handleEvent got actionId:",
      actionId,
      "for event:",
      eventName,
      "on component:",
      component.id
      // "collectFrom:",
      // collectFrom
    );

    // Skip normal handling for Google login button — it's handled by Google JS SDK
    if (component.id === "googleLoginButton") {
      console.log("[GoogleAuth] Google button click — skipped normal handler");
      return;
    }

    if (e?.preventDefault && (eventName === "onSubmit" || e.type === "submit")) {
      e.preventDefault();
    }

    // Collect only form input values from the page (not div textContent)
    const pageData = getEntirePage(component.id);

    await postClientMessageToCPP({
      componentId: component.id || "unknown",
      eventType: eventName,
      actionId,
      eventName,
      data: pageData,
      timestamp: Date.now(),
    });

    // 🔹 Instantly refresh the page after sending event
    window.reloadPage?.();

  } catch (error: unknown) {
    console.error(`Error handling event ${eventName}:`, error);
  }
}
