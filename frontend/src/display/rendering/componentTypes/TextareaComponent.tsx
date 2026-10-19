import React, { useLayoutEffect, useRef } from "react";
import { BaseComponentProps } from "./TextComponent";
import { keepsDraft, getDraft, setDraft } from "../../viewMemory";
import { layoutClasses } from "../buildClassName";
import handleEvent from "../../events/handleEvent";

interface TextareaComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    name?: string;
    attributes?: { [key: string]: any };
  };
}

const TextareaComponent: React.FC<TextareaComponentProps> = ({ component, className, style }) => {
  // What the reader has typed and not sent, for the boxes that keep a draft
  // (viewMemory.ts): put back after every render, so neither the AI-job poll
  // nor a remount (Debate <-> Brief) wipes it. The DOM value stays the one
  // getEntirePage collects and sends.
  const ref = useRef<HTMLTextAreaElement>(null);
  const draftable = keepsDraft(component.id);
  useLayoutEffect(() => {
    if (!draftable || !ref.current || !component.id) return;
    const draft = getDraft(component.id);
    if (draft !== undefined && ref.current.value !== draft) ref.current.value = draft;
  });
  const handleInput = (e: React.FormEvent<HTMLTextAreaElement>) => {
    if (draftable && component.id) setDraft(component.id, e.currentTarget.value);
  };

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
        ref={ref}
        onInput={handleInput}
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
