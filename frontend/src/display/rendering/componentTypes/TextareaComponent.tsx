import React from "react";
import { BaseComponentProps } from "./TextComponent";
import { layoutClasses } from "../buildClassName";
import handleEvent from "../../events/handleEvent";

interface TextareaComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    name?: string;
    attributes?: { [key: string]: any };
  };
}

const TextareaComponent: React.FC<TextareaComponentProps> = ({ component, className, style }) => {
  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
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
    <div className="flex flex-col gap-1">
      {component.text && (
        <label
          htmlFor={component.id}
          className="field-label"
        >
          {component.text}
        </label>
      )}
      <textarea
        id={component.id}
        name={component.name}
        defaultValue={component.value as string || ""}
        className={layoutClasses(className)}
        style={{
          ...style,
          minHeight: '64px',
          resize: 'vertical',
        }}
        onKeyDown={handleKeyDown}
        {...component.attributes}
      />
    </div>
  );
};

export default TextareaComponent;
