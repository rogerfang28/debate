import handleEvent from "../handleEvent.js";

export default function bindEvents(component) {
  const boundEvents = {};
  if (component.events) {
    for (const [eventName, actionId] of Object.entries(component.events)) {
      const reactEventName =
        eventName.charAt(0).toLowerCase() + eventName.slice(1);

      boundEvents[reactEventName] = (e) => {
        handleEvent(e, component, eventName, actionId, window.reloadPage);
      };
    }
  }
  return boundEvents;
}
