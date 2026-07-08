import React, { useState } from "react";
import { BaseComponentProps } from "./TextComponent";
import TreeComponent from "./TreeComponent";
import ParagraphTreeComponent from "./ParagraphTreeComponent";

type ViewMode = "tree" | "paragraph";

/**
 * TreeRootComponent owns the tree/paragraph view-mode toggle for the
 * top-level debate tree node, then delegates rendering to the matching
 * renderer. Both renderers walk the same underlying tree data.
 */
const TreeRootComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const [mode, setMode] = useState<ViewMode>("tree");

  const buttonStyle = (active: boolean): React.CSSProperties => ({
    fontSize: "0.7rem",
    padding: "0.15rem 0.5rem",
    borderRadius: "0.25rem",
    border: "1px solid #4b5563",
    background: active ? "#374151" : "transparent",
    color: active ? "#f3f4f6" : "#9ca3af",
    cursor: "pointer",
  });

  return (
    <div id={component.id} className={className} style={style}>
      <div style={{ display: "flex", gap: "0.375rem", marginBottom: "0.5rem" }}>
        <button style={buttonStyle(mode === "tree")} onClick={() => setMode("tree")}>
          Tree View
        </button>
        <button style={buttonStyle(mode === "paragraph")} onClick={() => setMode("paragraph")}>
          Paragraph View
        </button>
      </div>
      {mode === "tree" ? (
        <TreeComponent component={component} />
      ) : (
        <div style={{ lineHeight: 1.7, color: "#d1d5db", fontSize: "0.875rem" }}>
          <ParagraphTreeComponent component={component} />
        </div>
      )}
    </div>
  );
};

export default TreeRootComponent;
