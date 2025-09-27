import React from "react";
import { BaseComponentProps } from "./TextComponent";

interface TextareaComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    name?: string;
    attributes?: { [key: string]: any };
  };
}

const TextareaComponent: React.FC<TextareaComponentProps> = ({ component, className, style, events }) => {
  return (
    <textarea
      name={component.name}
      defaultValue={component.value as string || ""}
      className={className}
      style={style}
      {...events}
      {...component.attributes}
    />
  );
};

export default TextareaComponent;
