import React from "react";
// @ts-ignore - Generated protobuf file
import { ComponentType } from "../../../../src/gen/js/layout_pb.js";
import buildClassName, { ComponentStyle } from "./buildClassName.js";

import TextComponent from "./componentTypes/TextComponent.js";
import ButtonComponent from "./componentTypes/ButtonComponent.js";
import ImageComponent from "./componentTypes/ImageComponent.js";
import ListComponent from "./componentTypes/ListComponent.js";
import ContainerComponent from "./componentTypes/ContainerComponent.js";
import CardComponent from "./componentTypes/CardComponent.js";
import InputComponent from "./componentTypes/InputComponent.js";
import GraphComponent from "./componentTypes/GraphComponent.js";

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
  [ComponentType.IMAGE]: ImageComponent,
  [ComponentType.LIST]: ListComponent,
  [ComponentType.CONTAINER]: ContainerComponent,
  [ComponentType.CARD]: CardComponent,
  [ComponentType.INPUT]: InputComponent,
  [ComponentType.GRAPH]: GraphComponent,
};

const ComponentRenderer: React.FC<ComponentRendererProps> = ({ component }) => {
  if (!component) return null;

  const className = buildClassName(component.style);
  const inlineStyle: React.CSSProperties = component.css || {};

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
