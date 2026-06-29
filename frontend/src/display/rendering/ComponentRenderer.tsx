import React from "react";
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
import FormComponent from "./componentTypes/FormComponent.tsx";
import TableComponent from "./componentTypes/TableComponent.tsx";
import ModalComponent from "./componentTypes/ModalComponent.tsx";
import ChatComponent from "./componentTypes/ChatComponent.tsx";
import TextareaComponent from "./componentTypes/TextareaComponent.tsx";
import IconComponent from "./componentTypes/IconComponent.tsx";
import NodeGraphComponent from "./componentTypes/NodeGraphComponent.tsx";
import TreeComponent from "./componentTypes/TreeComponent.tsx";

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
  [ComponentType.FORM]: FormComponent,
  [ComponentType.TABLE]: TableComponent,
  [ComponentType.MODAL]: ModalComponent,
  [ComponentType.CHAT]: ChatComponent,
  [ComponentType.TEXTAREA]: TextareaComponent,
  [ComponentType.ICON]: IconComponent,
  [ComponentType.NODE_GRAPH]: NodeGraphComponent,
};

const ComponentRenderer: React.FC<ComponentRendererProps> = ({ component }) => {
  if (!component) return null;

  const className = buildClassName(component.style);
  const inlineStyle: React.CSSProperties = component.css || {};

  // Check if this is a tree node (CONTAINER with data-tree-node attribute)
  const attributes = component.attributes || {};
  if (component.type === ComponentType.CONTAINER && attributes["data-tree-node"] === "true") {
    return (
      <TreeComponent
        component={component}
        className={className}
        style={inlineStyle}
      />
    );
  }

  const Component = componentMap[component.type];

  if (!Component) {
    // Fallback: render as container with raw text
    return (
      <div id={component.id} className={className} style={inlineStyle}>
        {component.text || ''}
      </div>
    );
  }

  return (
    <Component
      component={component}
      className={className}
      style={inlineStyle}
    />
  );
};

export default ComponentRenderer;
