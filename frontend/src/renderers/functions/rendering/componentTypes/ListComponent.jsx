import React from "react";

export default function ListComponent({ component, className, style, events }) {
  return (
    <ul className={className} style={style} {...events}>
      {component.items?.map((item, idx) => (
        <li key={idx} className="list-item">
          {item.icon && <span className={`icon ${item.icon}`} />} {item.label}
        </li>
      ))}
    </ul>
  );
}
