import React from "react";

// Shared TypeScript interfaces for all components
export interface ComponentProps {
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
  events?: { [key: string]: (e: React.SyntheticEvent) => void };
}

const TextComponent: React.FC<BaseComponentProps> = ({ component, className, style, events }) => {
  return (
    <p className={className} style={style} {...events}>
      {component.text}
    </p>
  );
};

export default TextComponent;
