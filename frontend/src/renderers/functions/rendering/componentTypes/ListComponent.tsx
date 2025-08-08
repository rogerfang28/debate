import React from "react";
import { BaseComponentProps } from "./TextComponent";
import buildClassName from "../buildClassName";
import bindEvents from "../bindEvents";

interface ListItem {
  icon?: string;
  label?: string;
  style?: any;
  events?: any;
  [key: string]: any;
}

const ListComponent: React.FC<BaseComponentProps> = ({ component, className, style, events }) => {
  return (
    <ul className={className} style={style} {...events}>
      {component.items?.map((item: ListItem, idx: number) => {
<<<<<<< HEAD
        // Build className and events for each individual item
=======
>>>>>>> 6d99075 (tailwind works for now, trying to fix some hover)
        const itemClassName = buildClassName(item.style);
        const itemEvents = bindEvents(item);
        
        return (
          <li key={idx} className={itemClassName} {...itemEvents}>
            {item.icon && <span className={`icon ${item.icon}`} />} {item.label}
          </li>
        );
      })}
    </ul>
  );
};

export default ListComponent;
