import React from "react";

export default function IconComponent({ component, className, style, events }) {
  return (
    <span
      className={`${component.value || ""} ${className}`}
      style={style}
      {...events}
      {...component.attributes}
    >
      {component.text}
    </span>
  );
}
