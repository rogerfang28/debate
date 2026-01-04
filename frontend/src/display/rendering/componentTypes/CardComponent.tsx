import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

const CardComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <div id={component.id} className={`card p-4 shadow rounded ${className}`} style={style}>
      {component.children?.map((child, idx: number) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </div>
  );
};

export default CardComponent;
