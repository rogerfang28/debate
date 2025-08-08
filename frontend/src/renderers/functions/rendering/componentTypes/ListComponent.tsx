import React from "react";
import { BaseComponentProps } from "./TextComponent";

interface ListItem {
  icon?: string;
  label?: string;
  [key: string]: any;
}

const ListComponent: React.FC<BaseComponentProps> = ({ component, className, style, events }) => {
  return (
    <ul className={className} style={style} {...events}>
      {component.items?.map((item: ListItem, idx: number) => (
        <li key={idx} className="list-item">
          {item.icon && <span className={`icon ${item.icon}`} />} {item.label}
        </li>
      ))}
    </ul>
  );
};

export default ListComponent;
