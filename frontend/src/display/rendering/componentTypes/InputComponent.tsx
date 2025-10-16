import React from "react";
import { BaseComponentProps } from "./TextComponent";
// @ts-ignore - Generated protobuf file
import { EventType } from "../../../../../src/gen/js/event_pb.js";
import { getEntirePage } from "../../getEntirePage";

interface InputAttributes {
  type?: string;
  [key: string]: any;
}

interface InputComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    name?: string;
    attributes?: InputAttributes;
  };
}

const InputComponent: React.FC<InputComponentProps> = ({ component, className, style }) => {
  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      console.log(`⏎ Input "${component.id}" submitted with value:`, e.currentTarget.value);
      
      // Collect all page data and create event
      const eventData = getEntirePage(component.id || 'unknown-input', EventType.FORM_SUBMIT);
      
      // TODO: Send to backend
      console.log(`📦 Collected ${eventData.getEventDataList().length} fields from page`);
    }
  };

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    console.log(`✏️ Input "${component.id}" changed to:`, e.target.value);
    // Note: We don't collect full page data on every keystroke, only on Enter
  };

  return (
    <input
      id={component.id}
      type={component.attributes?.type || "text"}
      name={component.name}
      defaultValue={component.value as string || ""}
      placeholder={component.text || ""}
      className={className}
      style={style}
      onKeyDown={handleKeyDown}
      onChange={handleChange}
      {...component.attributes}
    />
  );
};

export default InputComponent;
