import React from "react";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";

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
      handleEvent(
        e as any,
        component,
        'onSubmit',
        component.id || 'unknown-input'
      );
    }
  };

  const inputType = component.attributes?.type || "text";

  if (inputType === 'checkbox' || inputType === 'radio') {
    return (
      <label className="flex items-center gap-2 cursor-pointer" style={{ fontSize: '0.875rem' }}>
        <input
          id={component.id}
          type={inputType}
          name={component.name}
          defaultChecked={component.value as boolean || false}
          className={className}
          style={style}
          {...component.attributes}
        />
        <span style={{ color: 'var(--text-secondary)' }}>{component.text}</span>
      </label>
    );
  }

  return (
    <div className="flex flex-col gap-1">
      {component.text && inputType !== 'hidden' && (
        <label
          htmlFor={component.id}
          className="text-xs font-medium"
          style={{ color: 'var(--text-secondary)' }}
        >
          {component.text}
        </label>
      )}
      <input
        id={component.id}
        type={inputType}
        name={component.name}
        defaultValue={component.value as string || ""}
        placeholder={component.placeholder || ""}
        className={className}
        style={style}
        onKeyDown={handleKeyDown}
        autoComplete="off"
        {...component.attributes}
      />
    </div>
  );
};

export default InputComponent;
