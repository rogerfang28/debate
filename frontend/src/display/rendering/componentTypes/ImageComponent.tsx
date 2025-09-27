import React from "react";
import { BaseComponentProps } from "./TextComponent";

const ImageComponent: React.FC<BaseComponentProps> = ({ component, className, style, events }) => {
  return (
    <img
      src={component.value as string}
      alt={component.text || ""}
      className={className}
      style={style}
      {...events}
    />
  );
};

export default ImageComponent;
