import React from "react";
import { BaseComponentProps } from "./TextComponent";

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
      // Input-specific Enter key behavior
    }
  };

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    console.log(`✏️ Input "${component.id}" changed to:`, e.target.value);
    // Input change behavior
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
