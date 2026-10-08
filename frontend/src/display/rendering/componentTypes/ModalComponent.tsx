import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

// The overlay and the dialog are theme.html's modal: .modal-overlay and
// .modal-content in index.css carry the look.
const ModalComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <div
      id={component.id}
      className={`modal-overlay ${className || ''}`}
      style={style}
    >
      <div className="modal-content slide-up">
        {component.children?.map((child, idx: number) => (
          <ComponentRenderer key={child.id || idx} component={child} />
        ))}
      </div>
    </div>
  );
};

export default ModalComponent;
