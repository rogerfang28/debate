import React from "react";

export default function UnknownComponent({ component }) {
  return (
    <div style={{ border: "1px dashed red", padding: "8px" }}>
      Unknown component type: {component.type}
    </div>
  );
}
