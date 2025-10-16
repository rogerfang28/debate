import React from "react";
import { BaseComponentProps } from "./TextComponent";
// @ts-ignore - Generated protobuf file
import { EventType } from "../../../../../src/gen/js/event_pb.js";
import { getEntirePage } from "../../getEntirePage";

const ButtonComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const handleClick = () => {
    console.log(`🖱️ Button "${component.id}" clicked`);
    
    // Collect all page data and create event
    const eventData = getEntirePage(component.id || 'unknown-button', EventType.CLICK);
    
    // TODO: Send to backend
    // For now, just log the collected data
    console.log(`📦 Collected ${eventData.getEventDataList().length} fields from page`);
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
