import React from "react";

export default function TextareaComponent({ component, className, style, events }) {
  return (
    <textarea
      name={component.name}
      defaultValue={component.value || ""}
      className={className}
      style={style}
      {...events}
      {...component.attributes}
    />
  );
}
