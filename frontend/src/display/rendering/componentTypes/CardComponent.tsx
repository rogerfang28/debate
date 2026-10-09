import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";
import { childKeys } from "../childKeys";

const CardComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const children = component.children || [];
  const keys = childKeys(component.id, children);
  return (
    <div
      id={component.id}
      className={`card ${className || ''}`}
      style={{
        ...style,
        background: 'var(--surface-card)',
        border: '1px solid var(--border-subtle)',
        borderRadius: 'var(--radius-lg)',
        padding: 'var(--space-lg)',
        overflow: 'hidden',
        wordBreak: 'break-word' as const,
      }}
    >
      {children.map((child, idx: number) => (
        <ComponentRenderer key={keys[idx]} component={child} />
      ))}
    </div>
  );
};

export default CardComponent;
