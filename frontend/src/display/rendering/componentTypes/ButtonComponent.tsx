import React from "react";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";

const ButtonComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const handleClick = (e: React.MouseEvent) => {
    console.log(`🖱️ Button "${component.id}" clicked`);
    
    // Send event to backend using the existing event handler
    handleEvent(
      e as any,
      component,
      'onClick',
      component.id || 'unknown-button'
    );
  };

  return (
    <button 
      id={component.id} 
      className={className} 
      style={style}
      onClick={handleClick}
    >
      {component.text}
    </button>
  );
};

export default ButtonComponent;
