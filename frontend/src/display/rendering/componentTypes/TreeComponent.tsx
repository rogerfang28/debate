import React, { useState, useCallback } from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

/**
 * TreeComponent renders a nested debate tree with expand/collapse.
 * 
 * It handles CONTAINER components that have data-tree-node="true" attribute.
 * - Clicking a node label toggles collapse of its children.
 * - Parent nodes show ▸ (collapsed) or ▾ (expanded) marker.
 * - Leaf nodes show no marker.
 */
const TreeComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  // Parse attributes from the component
  const attributes = component.attributes || {};
  const isTreeNode = attributes["data-tree-node"] === "true";
  
  // Find the label text and children container from the component's children
  const nested = component.children || component.components || [];
  const labelChild = nested.find((c: any) => (c.attributes || {})["data-tree-label"] === "true");
  const childrenContainer = nested.find((c: any) => (c.attributes || {})["data-tree-children"] === "true");
  const hasChildren = !!childrenContainer;

  // Collapse state: Set of node IDs that are collapsed
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

  return (
    <div
      id={component.id}
      className={className}
      style={{
        ...style,
        display: 'flex',
        flexDirection: 'column',
      }}
    >
      {/* Node label row with collapse toggle */}
      <div
        onClick={toggleCollapse}
        style={{
          display: 'flex',
          alignItems: 'center',
          gap: '0.25rem',
          cursor: hasChildren ? 'pointer' : 'default',
          userSelect: 'none',
        }}
      >
        {/* Collapse marker */}
        {hasChildren && (
          <span
            style={{
              fontSize: '0.75rem',
              color: '#6b7280',
              width: '1rem',
              textAlign: 'center',
              flexShrink: '0',
              transition: 'transform 0.15s ease',
            }}
          >
            {isCollapsed ? '▸' : '▾'}
          </span>
        )}
        {!hasChildren && (
          <span style={{ width: '1rem', flexShrink: '0' }} />
        )}
        {/* Label text */}
        {labelChild && (
          <ComponentRenderer component={labelChild} />
        )}
      </div>

      {/* Children (hidden when collapsed) */}
      {hasChildren && !isCollapsed && (
        <div
          style={{
            display: 'flex',
            flexDirection: 'column',
            marginLeft: '1.25rem',
            borderLeft: '1px solid #4b5563',
            paddingLeft: '0.75rem',
            gap: '0.25rem',
          }}
        >
          {(childrenContainer.children || []).map((child: any, idx: number) => (
            <ComponentRenderer key={child.id || idx} component={child} />
          ))}
        </div>
      )}
    </div>
  );
};

export default TreeComponent;
