import React from "react";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";

interface TextareaComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    name?: string;
    attributes?: { [key: string]: any };
  };
}

const TextareaComponent: React.FC<TextareaComponentProps> = ({ component, className, style }) => {
  const handleChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    console.log(`✏️ Textarea "${component.id}" changed to:`, e.target.value);
    
    // Send event to backend on change
    handleEvent(
      e as any,
      component,
      'onChange',
      component.id || 'unknown-textarea'
    );
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
