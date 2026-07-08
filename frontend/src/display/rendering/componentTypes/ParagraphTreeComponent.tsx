import React, { useState, useCallback } from "react";
import { BaseComponentProps } from "./TextComponent";

// Colors cycle by nesting depth so a marker, its sentence's underline, and
// the brackets wrapping its expanded children all share one color --
// making it visually obvious which children belong to which parent.
const DEPTH_COLORS = ["#60a5fa", "#34d399", "#f59e0b", "#f472b6", "#a78bfa", "#22d3ee"];

interface ParagraphTreeProps extends BaseComponentProps {
  depth?: number;
}

/**
 * ParagraphTreeComponent renders the same debate tree data as TreeComponent,
 * but as flowing run-on prose instead of an indented tree.
 *
 * Clicking a sentence span expands/collapses its children inline,
 * right after the sentence, in the same font/style as the rest of the text.
 */
const ParagraphTreeComponent: React.FC<ParagraphTreeProps> = ({ component, depth = 0 }) => {
  const nested = component.children || component.components || [];
  const labelChild = nested.find((c: any) => (c.attributes || {})["data-tree-label"] === "true");
  const childrenContainer = nested.find((c: any) => (c.attributes || {})["data-tree-children"] === "true");
  const childNodes = childrenContainer?.children || [];
  const hasChildren = childNodes.length > 0;

  const [collapsed, setCollapsed] = useState<Set<string>>(new Set());

  const toggleCollapse = useCallback((e: React.MouseEvent) => {
    e.stopPropagation();
    if (!hasChildren) return;
    setCollapsed(prev => {
      const next = new Set(prev);
      if (next.has(component.id)) {
        next.delete(component.id);
      } else {
        next.add(component.id);
      }
      return next;
    });
  }, [component.id, hasChildren]);

  const isCollapsed = collapsed.has(component.id);
  const text = labelChild?.text || "";
  const color = DEPTH_COLORS[depth % DEPTH_COLORS.length];

  return (
    <span id={component.id}>
      <span
        onClick={toggleCollapse}
        style={{
          cursor: hasChildren ? "pointer" : "default",
          userSelect: "none",
        }}
      >
        {hasChildren && (
          <span style={{ color, fontWeight: 700, marginRight: "0.15rem" }}>
            {isCollapsed ? "▸" : "▾"}
          </span>
        )}
        <span
          style={{
            fontWeight: hasChildren ? 600 : 400,
            borderBottom: hasChildren ? `2px solid ${color}` : "none",
          }}
        >
          {text}
        </span>
      </span>
      {" "}
      {hasChildren && !isCollapsed && (
        <>
          <span style={{ color, opacity: 0.7, fontWeight: 700 }}>[</span>
          {childNodes.map((child: any, idx: number) => (
            <ParagraphTreeComponent key={child.id || idx} component={child} depth={depth + 1} />
          ))}
          <span style={{ color, opacity: 0.7, fontWeight: 700 }}>]</span>
          {" "}
        </>
      )}
    </span>
  );
};

export default ParagraphTreeComponent;
