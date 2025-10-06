import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

const ContainerComponent: React.FC<BaseComponentProps> = ({ component, className, style, events }) => {
  return (
    <div className={className} style={style} {...events}>
      {component.children?.map((child, idx: number) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </div>
  );
};

export default ContainerComponent;
