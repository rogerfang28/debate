import React from "react";
// @ts-ignore - Generated protobuf file
import { ComponentType } from "../../../../../src/gen/ts/layout_pb.ts";

// Shared TypeScript interfaces for all components
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
  return (
    <p id={component.id} className={className} style={style}>
      {component.text}
    </p>
  );
};

export default TextComponent;
