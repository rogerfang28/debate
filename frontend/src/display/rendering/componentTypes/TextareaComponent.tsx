import React, { useLayoutEffect, useRef, useState } from "react";
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
    // Typed over: the box no longer holds just the loaded file.
    setFileNote("");
  };

  // `data-file-picker` (its value is the input's `accept` list) adds a
  // "Choose file" control that reads a local file into the box, in the
  // browser: the file is never uploaded on its own, its text is simply in
  // the box and goes with the next press, exactly as if it had been pasted.
  const pickerAccept = component.attributes?.["data-file-picker"] as string | undefined;
  const fileRef = useRef<HTMLInputElement>(null);
  const [fileNote, setFileNote] = useState("");
  const handleFile = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.currentTarget.files?.[0];
    e.currentTarget.value = "";  // choosing the same file again still loads it
    if (!file || !ref.current) return;
    const reader = new FileReader();
    reader.onload = () => {
      const box = ref.current;
      if (!box) return;
      box.value = typeof reader.result === "string" ? reader.result : "";
      // The same event typing sends, so anything listening sees a change.
      box.dispatchEvent(new Event("input", { bubbles: true }));
      setFileNote(`Loaded ${file.name} (${Math.max(1, Math.round(file.size / 1024))} KB)`);
    };
    reader.onerror = () => setFileNote(`Could not read ${file.name}`);
    reader.readAsText(file);
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
      {pickerAccept !== undefined && (
        <div className="flex items-center gap-2" style={{ flexWrap: "wrap" }}>
          <button
            type="button"
            onClick={() => fileRef.current?.click()}
            className="text-sm px-3 py-1 rounded"
            style={{ background: "transparent", border: "1px solid var(--line)", color: "var(--text-2)", cursor: "pointer" }}
          >
            Choose file
          </button>
          {/* No id: getEntirePage collects input[id], and this has no value to send. */}
          <input
            ref={fileRef}
            type="file"
            accept={pickerAccept || undefined}
            onChange={handleFile}
            style={{ display: "none" }}
            aria-label="Choose a file to load into the box"
          />
          {fileNote && (
            <span style={{ color: "var(--text-3)", fontSize: "12.5px" }}>{fileNote}</span>
          )}
        </div>
      )}
    </div>
  );
};

export default TextareaComponent;
