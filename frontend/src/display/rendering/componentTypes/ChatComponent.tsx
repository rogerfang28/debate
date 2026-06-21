import React from "react";
import { BaseComponentProps } from "./TextComponent";

const ChatComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const messages = component.items || [];

  return (
    <div
      id={component.id}
      className={`chat-component ${className || ''}`}
      style={{
        ...style,
        display: 'flex',
        flexDirection: 'column' as const,
        gap: '0.5rem',
        maxHeight: '400px',
        overflow: 'auto',
        padding: 'var(--space-md)',
        background: 'var(--surface-card)',
        border: '1px solid var(--border-subtle)',
        borderRadius: 'var(--radius-lg)',
      }}
    >
      {messages.map((message: any, idx: number) => (
        <div
          key={idx}
          className="chat-message"
          style={{
            padding: '0.5rem 0.75rem',
            background: 'var(--surface-elevated)',
            border: '1px solid var(--border-subtle)',
            borderRadius: 'var(--radius-md)',
            fontSize: '0.875rem',
            lineHeight: 1.5,
          }}
        >
          <strong style={{ color: 'var(--text-accent)', marginRight: '0.5rem' }}>
            {message.label}:
          </strong>
          <span style={{ color: 'var(--text-primary)' }}>{message.value}</span>
        </div>
      ))}
    </div>
  );
};

export default ChatComponent;
