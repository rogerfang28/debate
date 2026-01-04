import React from "react";
import { BaseComponentProps } from "./TextComponent";

const ChatComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <div id={component.id} className={`chat-component ${className}`} style={style}>
      {component.items?.map((message: any, idx: number) => (
        <div key={idx} className="chat-message">
          <strong>{message.label}:</strong> {message.value}
        </div>
      ))}
    </div>
  );
};

export default ChatComponent;
