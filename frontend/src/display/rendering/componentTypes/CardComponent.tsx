import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

const CardComponent: React.FC<BaseComponentProps> = ({ component, className, style, events }) => {
  return (
    <div className={`card p-4 shadow rounded ${className}`} style={style} {...events}>
      {component.children?.map((child, idx: number) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </div>
  );
};

export default CardComponent;
