import React from "react";
// @ts-ignore - Generated protobuf file
import { ComponentType } from "../../../../src/gen/ts/layout_pb.ts";
import buildClassName, { ComponentStyle } from "./buildClassName.ts";

import TextComponent from "./componentTypes/TextComponent.tsx";
import ButtonComponent from "./componentTypes/ButtonComponent.tsx";
import ImageComponent from "./componentTypes/ImageComponent.tsx";
import ListComponent from "./componentTypes/ListComponent.tsx";
import ContainerComponent from "./componentTypes/ContainerComponent.tsx";
import CardComponent from "./componentTypes/CardComponent.tsx";
import InputComponent from "./componentTypes/InputComponent.tsx";
import GraphComponent from "./componentTypes/GraphComponent.tsx";

// TypeScript interfaces
interface ComponentProps {
  id?: string;
  type: ComponentType;
  style?: ComponentStyle;
  css?: React.CSSProperties;
  [key: string]: any;
}

interface ComponentRendererProps {
  component: ComponentProps;
}

const componentMap: Partial<Record<ComponentType, React.ComponentType<any>>> = {
  [ComponentType.TEXT]: TextComponent,
  [ComponentType.BUTTON]: ButtonComponent,
  // [ComponentType.IMAGE]: ImageComponent,
  // [ComponentType.LIST]: ListComponent,
  [ComponentType.CONTAINER]: ContainerComponent,
  // [ComponentType.CARD]: CardComponent,
  [ComponentType.INPUT]: InputComponent,
  // [ComponentType.GRAPH]: GraphComponent,
};

const ComponentRenderer: React.FC<ComponentRendererProps> = ({ component }) => {
  if (!component) return null;

  const className = buildClassName(component.style);
  const inlineStyle: React.CSSProperties = component.css || {};

  // Debug logging for button components
  if (component.type === ComponentType.BUTTON) {
    console.log("Button component:", {
      id: component.id,
      style: component.style,
      className: className
    });
  }

  const Component = componentMap[component.type] || ContainerComponent;

  return (
    <Component
      component={component}
      className={className}
      style={inlineStyle}
    />
  );
};

export default ComponentRenderer;
