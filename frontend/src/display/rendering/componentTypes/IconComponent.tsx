import React from "react";
import { BaseComponentProps } from "./TextComponent";

const IconComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <span
      id={component.id}
      className={`${component.value || ""} ${className}`}
      style={style}
      {...component.attributes}
    >
      {component.text}
    </span>
  );
};

export default IconComponent;
