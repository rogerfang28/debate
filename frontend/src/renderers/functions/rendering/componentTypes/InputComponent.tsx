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

const InputComponent: React.FC<InputComponentProps> = ({ component, className, style, events }) => {
  return (
    <input
      type={component.attributes?.type || "text"}
      name={component.name}
      defaultValue={component.value as string || ""}
      className={className}
      style={style}
      {...events}
      {...component.attributes}
    />
  );
};

export default InputComponent;
