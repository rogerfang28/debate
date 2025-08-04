import React from "react";

export default function ButtonComponent({ component, className, style, events }) {
  return (
    <button className={className} style={style} {...events}>
      {component.text}
    </button>
  );
}
