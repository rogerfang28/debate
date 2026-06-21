import React from "react";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";

const ButtonComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const handleClick = (e: React.MouseEvent) => {
    console.log(`Button "${component.id}" clicked`);
    handleEvent(
      e as any,
      component,
      'onClick',
      component.id || 'unknown-button'
    );
  };

  const isSmall = className?.includes('text-xs') || className?.includes('text-sm');
  const isLarge = className?.includes('text-lg') || className?.includes('text-xl');
  const isRounded = className?.includes('rounded-full');

  return (
    <button
      id={component.id}
      className={className}
      style={{
        ...style,
        padding: isSmall
          ? '0.375rem 0.875rem'
          : isLarge
            ? '0.75rem 1.5rem'
            : '0.625rem 1.25rem',
        borderRadius: isRounded ? '9999px' : 'var(--radius-md)',
        fontSize: isSmall
          ? '0.8125rem'
          : isLarge
            ? '1rem'
            : '0.875rem',
        fontWeight: 500,
      }}
      onClick={handleClick}
    >
      {component.text}
    </button>
  );
};

export default ButtonComponent;
