import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

const ContainerComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const isFlex = className?.includes('flex');
  const isGrid = className?.includes('grid');

  return (
    <div
      id={component.id}
      className={className}
      style={{
        ...style,
        ...(!isFlex && !isGrid ? {
          display: 'flex',
          flexDirection: 'column' as const,
          gap: '0.75rem',
        } : {}),
        overflow: 'hidden',
      }}
    >
      {component.children?.map((child, idx: number) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </div>
  );
};

export default ContainerComponent;
