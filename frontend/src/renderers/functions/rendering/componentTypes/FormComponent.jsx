import React from "react";
import ComponentRenderer from "../ComponentRenderer.jsx";

export default function FormComponent({ component, className, style, events }) {
  return (
    <form className={className} style={style} {...events} {...component.attributes}>
      {component.children?.map((child, idx) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </form>
  );
}
