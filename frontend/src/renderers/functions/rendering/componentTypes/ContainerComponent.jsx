import React from "react";
import ComponentRenderer from "../ComponentRenderer.jsx";

export default function ContainerComponent({ component, className, style, events }) {
  return (
    <div className={className} style={style} {...events}>
      {component.children?.map((child, idx) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </div>
  );
}
