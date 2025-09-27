import React from "react";
import { ComponentType } from "../../../../src/gen/js/page_pb.js";
import buildClassName, { ComponentStyle } from "./buildClassName.js";
import bindEvents, { ComponentEvent } from "./bindEvents.ts";

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
  events?: ComponentEvent;
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
  const boundEvents = bindEvents(component);

  const Component = componentMap[component.type] || ContainerComponent;

  return (
    <Component
      component={component}
      className={className}
      style={inlineStyle}
      events={boundEvents}
    />
  );
};

export default ComponentRenderer;
