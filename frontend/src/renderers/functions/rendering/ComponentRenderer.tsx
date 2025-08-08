import React from "react";
import { ComponentType } from "../../../../../src/gen/page_pb.js";
import buildClassName, { ComponentStyle } from "./buildClassName";
import bindEvents, { ComponentEvent } from "./bindEvents";

// @ts-ignore - Component types not converted yet
import TextComponent from "./componentTypes/TextComponent.jsx";
// @ts-ignore - Component types not converted yet
import ButtonComponent from "./componentTypes/ButtonComponent.jsx";
// @ts-ignore - Component types not converted yet
import ImageComponent from "./componentTypes/ImageComponent.jsx";
// @ts-ignore - Component types not converted yet
import ListComponent from "./componentTypes/ListComponent.jsx";
// @ts-ignore - Component types not converted yet
import ContainerComponent from "./componentTypes/ContainerComponent.jsx";
// @ts-ignore - Component types not converted yet
import CardComponent from "./componentTypes/CardComponent.jsx";

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
