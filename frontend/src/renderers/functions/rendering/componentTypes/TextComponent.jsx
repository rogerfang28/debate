import React from "react";

export default function TextComponent({ component, className, style, events }) {
  return (
    <p className={className} style={style} {...events}>
      {component.text}
    </p>
  );
}
