import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

const ModalComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <div
      id={component.id}
      className={`modal-overlay ${className || ''}`}
      style={{
        ...style,
        position: 'fixed',
        inset: 0,
        zIndex: 50,
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        background: 'rgba(0, 0, 0, 0.6)',
        backdropFilter: 'blur(4px)',
      }}
    >
      <div
        className="modal-content slide-up"
        style={{
          background: 'var(--surface-card)',
          border: '1px solid var(--border-default)',
          borderRadius: 'var(--radius-xl)',
          padding: 'var(--space-xl)',
          maxWidth: '90vw',
          maxHeight: '85vh',
          overflow: 'auto',
          boxShadow: 'var(--shadow-lg)',
        }}
      >
        {component.children?.map((child, idx: number) => (
          <ComponentRenderer key={child.id || idx} component={child} />
        ))}
      </div>
    </div>
  );
};

export default ModalComponent;
