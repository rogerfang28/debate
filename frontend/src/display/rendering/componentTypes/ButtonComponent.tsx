import React from "react";
import { BaseComponentProps } from "./TextComponent";

const ButtonComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const handleClick = () => {
    console.log(`🖱️ Button "${component.id}" clicked`);
    // Button-specific behavior can be added here if needed
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
