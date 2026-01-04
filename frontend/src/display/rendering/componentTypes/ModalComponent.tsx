import React from "react";
import ComponentRenderer from "../ComponentRenderer.ts";
import { BaseComponentProps } from "./TextComponent";

const ModalComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <div id={component.id} className={`modal-overlay ${className}`} style={style}>
      <div className="modal-content">
        {component.children?.map((child, idx: number) => (
          <ComponentRenderer key={child.id || idx} component={child} />
        ))}
      </div>
    </div>
  );
};

export default ModalComponent;
