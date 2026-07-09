import React from "react";
import { BaseComponentProps } from "./TextComponent";
import ParagraphTreeComponent from "./ParagraphTreeComponent";

/**
 * TreeRootComponent renders the top-level debate tree as flowing paragraph
 * prose. Tree view is temporarily disabled -- paragraph view only for now.
 */
const TreeRootComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <div id={component.id} className={className} style={style}>
      <div style={{ lineHeight: 1.7, color: "#d1d5db", fontSize: "0.875rem" }}>
        <ParagraphTreeComponent component={component} />
      </div>
    </div>
  );
};

export default TreeRootComponent;
