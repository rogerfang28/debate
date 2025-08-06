import React from "react";
import { ComponentType } from "../../../../../protos/unused/page_pb.js";
import buildClassName from "./buildClassName.js";
import bindEvents from "./bindEvents.js";

import TextComponent from "./componentTypes/TextComponent.jsx";
import ButtonComponent from "./componentTypes/ButtonComponent.jsx";
import ImageComponent from "./componentTypes/ImageComponent.jsx";
import ListComponent from "./componentTypes/ListComponent.jsx";
import ContainerComponent from "./componentTypes/ContainerComponent.jsx";
import CardComponent from "./componentTypes/CardComponent.jsx";

const componentMap = {
  [ComponentType.TEXT]: TextComponent,
  [ComponentType.BUTTON]: ButtonComponent,
  [ComponentType.IMAGE]: ImageComponent,
  [ComponentType.LIST]: ListComponent,
  [ComponentType.CONTAINER]: ContainerComponent,
  [ComponentType.CARD]: CardComponent,
};

export default function ComponentRenderer({ component }) {
  if (!component) return null;

  const className = buildClassName(component.style);
  const inlineStyle = component.css || {};
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
}
