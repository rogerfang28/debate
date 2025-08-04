import React from "react";

export default function ImageComponent({ component, className, style, events }) {
  return (
    <img
      src={component.value}
      alt={component.text || ""}
      className={className}
      style={style}
      {...events}
    />
  );
}
