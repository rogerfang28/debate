import React from "react";
import { BaseComponentProps } from "./TextComponent";

const ButtonComponent: React.FC<BaseComponentProps> = ({ component, className, style, events }) => {
  return (
    <button id={component.id} className={className} style={style} {...events}>
      {component.text}
    </button>
  );
};

export default ButtonComponent;
