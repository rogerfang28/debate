import postClientMessageToCPP from "../../backendCommunicator/postClientMessageToCPP.ts";
import getEntirePage from "../getEntirePage.ts";
import { draftsSentBy } from "../viewMemory.ts";

// Google sign-in callback — set by GoogleLoginComponent
// eslint-disable-next-line @typescript-eslint/no-explicit-any
(window as any).googleLoginCallback = async (credential: string) => {
  console.log("[GoogleAuth] Got ID token from Google, sending to backend...");
  try {
    const page = await postClientMessageToCPP({
      componentId: "googleLoginButton",
      eventType: "onGoogleLogin",
      actionId: "google",
      eventName: "onGoogleLogin",
      data: { _pageId: "login", google_id_token: credential },
      timestamp: Date.now(),
    });
    if (page && window.applyPageData) {
      window.applyPageData(page);
    } else {
      window.reloadPage?.();
    }
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

// A button press is in flight. Agree, Withdraw, Challenge and the rest each
// change the debate, and the page answers only when the server has: a second
// press before then used to be sent too (five Withdraws in a second, each
// toggling the last). Presses of a <button> are dropped until the answer
// lands; the root carries data-busy meanwhile so the cursor says so. Clicks
// on the map (selecting a claim) are not buttons and always go through, so
// the selection keeps up with the camera.
let buttonInFlight = false;

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

    const fromButton = String(eventName) === "onClick" &&
      ((e as any)?.currentTarget?.tagName === "BUTTON" || (e as any)?.target?.closest?.("button") != null);
    if (fromButton && buttonInFlight) {
      console.log("handleEvent: a button press is already in flight; ignoring", component.id);
      return;
    }
    if (fromButton) {
      buttonInFlight = true;
      document.documentElement.dataset.busy = "1";
    }
    try {

    // Collect only form input values from the page (not div textContent)
    const pageData = getEntirePage(component.id);
    // The value is collected; if this event is what sends a kept draft (the
    // chat's Ask), the draft is spent and must not come back on next render.
    draftsSentBy(component.id || "", String(eventName));

    const page = await postClientMessageToCPP({
      componentId: component.id || "unknown",
      eventType: eventName,
      actionId,
      eventName,
      data: pageData,
      timestamp: Date.now(),
    });

    // Apply the page already returned by this event's own response instead
    // of firing a second, redundant full-page fetch via reloadPage().
    if (page && window.applyPageData) {
      window.applyPageData(page);
    } else {
      window.reloadPage?.();
    }
    } finally {
      if (fromButton) {
        buttonInFlight = false;
        delete document.documentElement.dataset.busy;
      }
    }

  } catch (error: unknown) {
    console.error(`Error handling event ${eventName}:`, error);
  }
}
