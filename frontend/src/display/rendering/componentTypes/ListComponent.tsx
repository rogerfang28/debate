import React from "react";
import { BaseComponentProps } from "./TextComponent";
import buildClassName from "../buildClassName";
// @ts-ignore - Generated protobuf file
import { EventType } from "../../../../../src/gen/js/event_pb.js";
import { getEntirePage } from "../../getEntirePage";

interface ListItem {
  icon?: string;
  label?: string;
  style?: any;
  [key: string]: any;
}

const ListComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const handleItemClick = (item: ListItem, idx: number) => {
    console.log(`📝 List "${component.id}" item ${idx} clicked:`, item.label);
    
    // Collect all page data and create event
    const eventData = getEntirePage(component.id || 'unknown-list', EventType.CLICK);
    
    // TODO: Send to backend with item info
    console.log(`📦 Collected ${eventData.getEventDataList().length} fields from page`);
  };

  return (
    <ul id={component.id} className={className} style={style}>
      {component.items?.map((item: ListItem, idx: number) => {
        // Build className for each individual item
        const itemClassName = buildClassName(item.style);
        
        return (
          <li 
            key={idx} 
            className={itemClassName}
            onClick={() => handleItemClick(item, idx)}
          >
            {item.icon && <span className={`icon ${item.icon}`} />} {item.label}
          </li>
        );
      })}
    </ul>
  );
};

export default ListComponent;
