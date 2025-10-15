import React from "react";
import { BaseComponentProps } from "./TextComponent";

const ImageComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <img
      id={component.id}
      src={component.value as string}
      alt={component.text || ""}
      className={className}
      style={style}
    />
  );
};

export default ImageComponent;
