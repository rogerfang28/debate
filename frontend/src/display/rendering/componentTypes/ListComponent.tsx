import React from "react";
import { BaseComponentProps } from "./TextComponent";
import buildClassName from "../buildClassName";
import handleEvent from "../../events/handleEvent";

interface ListItem {
  icon?: string;
  label?: string;
  style?: any;
  [key: string]: any;
}

const ListComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const handleItemClick = (e: React.MouseEvent, item: ListItem, idx: number) => {
    console.log(`📝 List "${component.id}" item ${idx} clicked:`, item.label);
    
    // Send event to backend on list item click
    handleEvent(
      e as any,
      { ...component, value: item.label, text: item.label },
      'onClick',
      `${component.id}-item-${idx}` || 'unknown-list-item'
    );
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
            onClick={(e) => handleItemClick(e, item, idx)}
          >
            {item.icon && <span className={`icon ${item.icon}`} />} {item.label}
          </li>
        );
      })}
    </ul>
  );
};

export default ListComponent;
