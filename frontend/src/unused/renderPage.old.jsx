// import React from "react";
// import { ComponentType } from "../../../../../protos/page_pb.js";
// import { EventType } from "../../../../../protos/event_pb.js"; // adjust path
// import handleEvent from "../handleEvent.js"; // new centralized function

// // Map backend event names → EventType enum
// export const eventTypeMap = {
//   onClick: EventType.CLICK,
//   onChange: EventType.INPUT_CHANGE,
//   onSubmit: EventType.FORM_SUBMIT,
// };

// export function PageRenderer({ page }) {
//   if (!page?.components) return null;

//   return (
//     <div className="page-root">
//       {page.components.map((component, idx) => (
//         <ComponentRenderer key={component.id || idx} component={component} />
//       ))}
//     </div>
//   );
// }

// function ComponentRenderer({ component }) {
//   if (!component) return null;

//   const { type, text, children, style, css, events, items } = component;

//   // Convert style object → Tailwind-friendly className string
//   const className = [
//     style?.customClass || "",
//     style?.padding || "",
//     style?.margin || "",
//     style?.bgColor || style?.bg_color || "",
//     style?.textColor || style?.text_color || "",
//     style?.border || "",
//     style?.rounded || "",
//     style?.shadow || "",
//     style?.gap || "",
//   ]
//     .filter(Boolean)
//     .join(" ");

//   const inlineStyle = css || {};

//   // Bind events to call handleEvent.js
//   const boundEvents = {};
//   if (events) {
//     for (const [eventName, actionId] of Object.entries(events)) {
//       const reactEventName =
//         eventName.charAt(0).toLowerCase() + eventName.slice(1);

//       boundEvents[reactEventName] = (e) => {
//         handleEvent(e, component, eventName, actionId, window.reloadPage);
//       };
//     }
//   }

//   // Render different component types
//   switch (type) {
//     case ComponentType.TEXT:
//       return <p className={className} style={inlineStyle} {...boundEvents}>{text}</p>;

//     case ComponentType.BUTTON:
//       return <button className={className} style={inlineStyle} {...boundEvents}>{text}</button>;

//     case ComponentType.IMAGE:
//       return <img src={component.value} alt={text || ""} className={className} style={inlineStyle} {...boundEvents} />;

//     case ComponentType.LIST:
//       return (
//         <ul className={className} style={inlineStyle} {...boundEvents}>
//           {items?.map((item, idx) => (
//             <li key={idx} className="list-item">
//               {item.icon && <span className={`icon ${item.icon}`} />} {item.label}
//             </li>
//           ))}
//         </ul>
//       );

//     case ComponentType.CONTAINER:
//       return (
//         <div className={className} style={inlineStyle} {...boundEvents}>
//           {children?.map((child, idx) => (
//             <ComponentRenderer key={child.id || idx} component={child} />
//           ))}
//         </div>
//       );

//     case ComponentType.CARD:
//       return (
//         <div className={`card p-4 shadow rounded ${className}`} style={inlineStyle} {...boundEvents}>
//           {children?.map((child, idx) => (
//             <ComponentRenderer key={child.id || idx} component={child} />
//           ))}
//         </div>
//       );

//     default:
//       return (
//         <div className={className} style={inlineStyle} {...boundEvents}>
//           {children?.map((child, idx) => (
//             <ComponentRenderer key={child.id || idx} component={child} />
//           ))}
//         </div>
//       );
//   }
// }
