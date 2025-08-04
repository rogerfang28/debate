import React from "react";

export default function NodeGraphComponent({ component, className, style, events }) {
  return (
    <div className={`node-graph ${className}`} style={style} {...events}>
      {/* Placeholder for node graph visualization */}
      {component.text || "Node Graph"}
    </div>
  );
}
