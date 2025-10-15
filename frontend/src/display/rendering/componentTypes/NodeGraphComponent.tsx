import React from "react";
import { BaseComponentProps } from "./TextComponent";

const NodeGraphComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <div className={`node-graph ${className}`} style={style}>
      {/* Placeholder for node graph visualization */}
      {component.text || "Node Graph"}
    </div>
  );
};

export default NodeGraphComponent;
