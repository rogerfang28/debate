import React from "react";
import { BaseComponentProps } from "./TextComponent";
import buildClassName from "../buildClassName";

interface ListItem {
  icon?: string;
  label?: string;
  style?: any;
  [key: string]: any;
}

const ListComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const handleItemClick = (item: ListItem, idx: number) => {
    console.log(`📝 List item ${idx} clicked:`, item.label);
    // List item click behavior
  };

  return (
    <ul className={className} style={style}>
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
