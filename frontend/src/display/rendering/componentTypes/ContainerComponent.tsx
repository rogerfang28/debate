import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

const ContainerComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <div className={className} style={style}>
      {component.children?.map((child, idx: number) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </div>
  );
};

export default ContainerComponent;
