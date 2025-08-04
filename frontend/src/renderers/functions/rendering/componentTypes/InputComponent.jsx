import React from "react";

export default function InputComponent({ component, className, style, events }) {
  return (
    <input
      type={component.attributes?.type || "text"}
      name={component.name}
      defaultValue={component.value || ""}
      className={className}
      style={style}
      {...events}
      {...component.attributes}
    />
  );
}
