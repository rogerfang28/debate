import React from "react";
import { BaseComponentProps } from "./TextComponent";

interface TextareaComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    name?: string;
    attributes?: { [key: string]: any };
  };
}

const TextareaComponent: React.FC<TextareaComponentProps> = ({ component, className, style }) => {
  const handleChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    console.log(`✏️ Textarea "${component.name}" changed to:`, e.target.value);
    // Textarea change behavior
  };

  return (
    <textarea
      id={component.id}
      name={component.name}
      defaultValue={component.value as string || ""}
      className={className}
      style={style}
      onChange={handleChange}
      {...component.attributes}
    />
  );
};

export default TextareaComponent;
