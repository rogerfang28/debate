import React from "react";
import { BaseComponentProps } from "./TextComponent";

const NodeGraphComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <div
      id={component.id}
      className={`node-graph ${className || ''}`}
      style={{
        ...style,
        background: 'var(--surface-card)',
        border: '1px solid var(--border-subtle)',
        borderRadius: 'var(--radius-lg)',
        padding: 'var(--space-lg)',
        minHeight: '200px',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
      }}
    >
      <div className="text-center">
        <div
          className="w-16 h-16 mx-auto mb-4 rounded-full flex items-center justify-center"
          style={{ background: 'var(--surface-elevated)', border: '1px solid var(--border-default)' }}
        >
          <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="var(--text-muted)" strokeWidth="1.5">
            <circle cx="12" cy="12" r="3" />
            <circle cx="5" cy="5" r="2" />
            <circle cx="19" cy="5" r="2" />
            <circle cx="5" cy="19" r="2" />
            <circle cx="19" cy="19" r="2" />
            <line x1="9.5" y1="10" x2="6.5" y2="6.5" />
            <line x1="14.5" y1="10" x2="17.5" y2="6.5" />
            <line x1="9.5" y1="14" x2="6.5" y2="17.5" />
            <line x1="14.5" y1="14" x2="17.5" y2="17.5" />
          </svg>
        </div>
        <p className="text-sm" style={{ color: 'var(--text-muted)' }}>
          {component.text || "Node Graph"}
        </p>
      </div>
    </div>
  );
};

export default NodeGraphComponent;
