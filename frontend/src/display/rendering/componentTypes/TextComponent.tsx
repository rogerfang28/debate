import React from "react";
import { ComponentType } from "../../../../../src/gen/ts/layout_pb.ts";

export interface ComponentProps {
  type: ComponentType;
  text?: string;
  value?: string | number | boolean;
  placeholder?: string;
  src?: string;
  alt?: string;
  href?: string;
  items?: any[];
  children?: ComponentProps[];
  [key: string]: any;
}

export interface BaseComponentProps {
  component: ComponentProps;
  className?: string;
  style?: React.CSSProperties;
}

const TextComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const text = component.text || "";

  // Detect if text looks like a heading (short, no period, or marked)
  const isHeading = text.length < 80 && !text.endsWith('.') && !text.endsWith(',');
  const isMuted = className?.includes('text-gray-') || className?.includes('text-muted');

  if (isHeading && !isMuted) {
    return (
      <h3
        id={component.id}
        className={className}
        style={{
          ...style,
          color: 'var(--text-primary)',
          fontWeight: 600,
          fontSize: '1rem',
          lineHeight: 1.4,
          marginBottom: '0.25rem',
        }}
      >
        {text}
      </h3>
    );
  }

  return (
    <p
      id={component.id}
      className={className}
      style={{
        ...style,
        color: isMuted ? 'var(--text-muted)' : 'var(--text-secondary)',
        fontSize: '0.875rem',
        lineHeight: 1.6,
      }}
    >
      {text}
    </p>
  );
};

export default TextComponent;
