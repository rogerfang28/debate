import React from "react";
import ComponentRenderer from "../ComponentRenderer.tsx";

export default function CardComponent({ component, className, style, events }) {
  return (
    <div className={`card p-4 shadow rounded ${className}`} style={style} {...events}>
      {component.children?.map((child, idx) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </div>
  );
}
