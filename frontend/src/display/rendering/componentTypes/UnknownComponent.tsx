import React from "react";
import { BaseComponentProps } from "./TextComponent";

const UnknownComponent: React.FC<BaseComponentProps> = ({ component }) => {
  return (
    <div style={{ border: "1px dashed red", padding: "8px" }}>
      Unknown component type: {component.type}
    </div>
  );
};

export default UnknownComponent;
