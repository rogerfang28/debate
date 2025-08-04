// frontend/src/functions/renderPage.jsx
import React from "react";
import { ComponentType } from "../../../../protos/page_pb.js";
import sendDataToBackend from "./sendData.js";
import { EventType } from "../../../../protos/event_pb.js"; // adjust path

// Optional: still allow special frontend-only events
const eventHandlers = {
  goProfile: () => alert("Go to profile"),
  startExperience: () => alert("Starting experience..."),
  sayHello: () => alert("Hello from backend event!"),
};

// Map string event names from backend → EventType enum
const eventTypeMap = {
  onClick: EventType.CLICK,
  onChange: EventType.INPUT_CHANGE,
  onSubmit: EventType.FORM_SUBMIT,
};

export default function renderPage(page) {
  if (!page?.components) return null;

  return (
    <div className="page-root">
      {page.components.map((component, idx) => (
        <ComponentRenderer key={component.id || idx} component={component} />
      ))}
    </div>
  );
}

function ComponentRenderer({ component }) {
  if (!component) return null;

  const { type, text, children, style, css, events, items } = component;

  const className = [
    style?.customClass || "",
    style?.padding || "",
    style?.margin || "",
    style?.bgColor || style?.bg_color || "",
    style?.textColor || style?.text_color || "",
    style?.border || "",
    style?.rounded || "",
    style?.shadow || "",
    style?.gap || "",
  ]
    .filter(Boolean)
    .join(" ");

  const inlineStyle = css || {};

  // Bind events
  const boundEvents = {};
  if (events) {
    for (const [eventName, handlerName] of Object.entries(events)) {
      const reactEventName =
        eventName.charAt(0).toLowerCase() + eventName.slice(1);

      boundEvents[reactEventName] = (e) => {
        // Prevent default if it's a form submit
        if (e?.preventDefault) e.preventDefault();

        // 1️⃣ Send to backend
        sendDataToBackend({
          componentId: component.id || "unknown",
          type: eventTypeMap[eventName] || EventType.UNKNOWN,
          data: { text, value: e?.target?.value }
        });

        // 2️⃣ Call local handler if it exists
        if (eventHandlers[handlerName]) {
          eventHandlers[handlerName](e);
        }
      };
    }
  }

  switch (type) {
    case ComponentType.TEXT:
      return <p className={className} style={inlineStyle} {...boundEvents}>{text}</p>;

    case ComponentType.BUTTON:
      return <button className={className} style={inlineStyle} {...boundEvents}>{text}</button>;

    case ComponentType.IMAGE:
      return <img src={component.value} alt={text || ""} className={className} style={inlineStyle} {...boundEvents} />;

    case ComponentType.LIST:
      return (
        <ul className={className} style={inlineStyle} {...boundEvents}>
          {items?.map((item, idx) => (
            <li key={idx} className="list-item">
              {item.icon && <span className={`icon ${item.icon}`} />} {item.label}
            </li>
          ))}
        </ul>
      );

    case ComponentType.CONTAINER:
      return (
        <div className={className} style={inlineStyle} {...boundEvents}>
          {children?.map((child, idx) => (
            <ComponentRenderer key={child.id || idx} component={child} />
          ))}
        </div>
      );

    case ComponentType.CARD:
      return (
        <div className={`card p-4 shadow rounded ${className}`} style={inlineStyle} {...boundEvents}>
          {children?.map((child, idx) => (
            <ComponentRenderer key={child.id || idx} component={child} />
          ))}
        </div>
      );

    default:
      return (
        <div className={className} style={inlineStyle} {...boundEvents}>
          {children?.map((child, idx) => (
            <ComponentRenderer key={child.id || idx} component={child} />
          ))}
        </div>
      );
  }
}
