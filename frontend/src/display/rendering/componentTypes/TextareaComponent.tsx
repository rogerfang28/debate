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
  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    // Send event on Ctrl+Enter or Cmd+Enter
    if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) {
      console.log(`⏎ Textarea "${component.id}" submitted with value:`, e.currentTarget.value);
      
      handleEvent(
        e as any,
        component,
        'onSubmit',
        component.id || 'unknown-textarea'
      );
    }
  };

  return (
    <textarea
      id={component.id}
      name={component.name}
      defaultValue={component.value as string || ""}
      className={className}
      style={style}
      onKeyDown={handleKeyDown}
      {...component.attributes}
    />
  );
};

export default TextareaComponent;
