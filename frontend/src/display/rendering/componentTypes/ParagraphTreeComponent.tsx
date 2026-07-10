import React, { useState, useCallback } from "react";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";

// Colors cycle by nesting depth so a marker, its sentence's underline, and
// the brackets wrapping its expanded children all share one color --
// making it visually obvious which children belong to which parent.
const DEPTH_COLORS = ["#60a5fa", "#34d399", "#f59e0b", "#f472b6", "#a78bfa", "#22d3ee"];

// Matches the "Challenge" color used in the debate map/graph, so a
// challenging claim reads visually the same way in both views.
const CHALLENGE_COLOR = "#f97316";

// Matches the "current claim" gold highlight used in the debate map/graph.
const CURRENT_COLOR = "#fbbf24";

interface ParagraphTreeProps extends BaseComponentProps {
  depth?: number;
}

// Ensures each sentence reads like actual prose -- appends a period if the
// text doesn't already end with sentence-ending punctuation.
function ensureSentenceEnd(text: string): string {
  const trimmed = text.trim();
  if (!trimmed) return trimmed;
  return /[.!?]["')\]]?$/.test(trimmed) ? trimmed : trimmed + ".";
}

/**
 * ParagraphTreeComponent renders the same debate tree data as TreeComponent,
 * but as flowing run-on prose instead of an indented tree.
 *
 * Clicking the ▸/▾ marker expands/collapses a node's children. Clicking the
 * sentence itself selects that claim (server round-trip via SELECT_CLAIM),
 * which moves the "current claim" highlight to it on next render.
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

  const claimId = (component.attributes || {})["data-node-id"];
  const selectClaim = useCallback((e: React.MouseEvent) => {
    e.stopPropagation();
    if (!claimId) return;
    const syntheticId = `selectClaimNode_${claimId}`;
    handleEvent(e as any, { id: syntheticId }, "onClick", syntheticId);
  }, [claimId]);

  const isCollapsed = collapsed.has(component.id);
  const text = ensureSentenceEnd(labelChild?.text || "");
  const isChallenge = (component.attributes || {})["data-tree-challenge"] === "true";
  const isCurrent = (component.attributes || {})["data-tree-current"] === "true";
  const color = isChallenge ? CHALLENGE_COLOR : DEPTH_COLORS[depth % DEPTH_COLORS.length];

  return (
    <span id={component.id}>
      <span style={{ userSelect: "none" }}>
        {hasChildren && (
          <span
            onClick={toggleCollapse}
            style={{ color, fontWeight: 700, marginRight: "0.15rem", cursor: "pointer" }}
          >
            {isCollapsed ? "▸" : "▾"}
          </span>
        )}
        {isChallenge && (
          <span style={{ color, fontWeight: 700, fontStyle: "italic", marginRight: "0.25rem" }}>
            Challenge:
          </span>
        )}
        <span
          onClick={selectClaim}
          title="Click to select this claim"
          style={{
            fontWeight: hasChildren || isChallenge ? 600 : 400,
            fontStyle: isChallenge ? "italic" : "normal",
            borderBottom: hasChildren ? `2px solid ${color}` : "none",
            cursor: "pointer",
            background: isCurrent ? "rgba(251, 191, 36, 0.18)" : "transparent",
            outline: isCurrent ? `1px solid ${CURRENT_COLOR}` : "none",
            borderRadius: isCurrent ? "0.25rem" : "0",
            padding: isCurrent ? "0 0.15rem" : "0",
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
