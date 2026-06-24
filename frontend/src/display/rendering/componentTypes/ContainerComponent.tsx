import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

const ContainerComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const isFlex = className?.includes('flex');
  const isGrid = className?.includes('grid');
  // Support both "children" (from protobuf) and "components" (from pbtxt) for nesting
  const nested = component.children || component.components || [];

  return (
    <div
      id={component.id}
      className={className}
      style={{
        ...(!isFlex && !isGrid ? {
          display: 'flex',
          flexDirection: 'column' as const,
          gap: '0.75rem',
        } : {}),
        ...style,
        overflow: style?.overflow || 'hidden',
      }}
    >
      {nested.map((child: any, idx: number) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </div>
  );
};

export default ContainerComponent;
