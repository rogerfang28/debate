import React from "react";
import ComponentRenderer from "../ComponentRenderer.jsx";

export default function ModalComponent({ component, className, style, events }) {
  return (
    <div className={`modal-overlay ${className}`} style={style} {...events}>
      <div className="modal-content">
        {component.children?.map((child, idx) => (
          <ComponentRenderer key={child.id || idx} component={child} />
        ))}
      </div>
    </div>
  );
}
