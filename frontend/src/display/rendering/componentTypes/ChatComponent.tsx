import React from "react";

export default function ChatComponent({ component, className, style, events }) {
  return (
    <div className={`chat-component ${className}`} style={style} {...events}>
      {component.items?.map((message, idx) => (
        <div key={idx} className="chat-message">
          <strong>{message.label}:</strong> {message.value}
        </div>
      ))}
    </div>
  );
}
