import handleEvent from "../handleEvent.js";

/**
 * Binds backend-defined events to React event handlers
 * so that they call handleEvent() with the correct actionId.
 */
export default function bindEvents(component) {
  const boundEvents = {};

  if (!component?.events) return boundEvents; // no events to bind

  for (const [eventName, actionId] of Object.entries(component.events)) {
    // Convert backend event names to valid React event prop names
    let reactEventName;

    if (eventName === "onEnter") {
      // Special case: map "onEnter" to React's onKeyDown
      reactEventName = "onKeyDown";
    } else {
      // Ensure the event name starts lowercase (React props are camelCase)
      reactEventName = eventName.charAt(0).toLowerCase() + eventName.slice(1);
    }

    boundEvents[reactEventName] = (e) => {
      // Special handling for "onEnter"
      if (eventName === "onEnter" && e.key !== "Enter") return;

      // Pass full event info to handleEvent
      handleEvent(
        e,
        component,
        eventName,   // backend event name ("onClick", "onEnter")
        actionId,    // this comes directly from component.events map ("goProfile", etc.)
        window.reloadPage
      );
    };
  }

  return boundEvents;
}
