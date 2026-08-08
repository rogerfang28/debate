import React, { useState } from "react";
import { BaseComponentProps } from "./TextComponent";

interface MultiInputAttributes {
  placeholder?: string;
  extraPlaceholder?: string;
  addLabel?: string;
  maxInputs?: string;
  [key: string]: any;
}

interface MultiInputComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    name?: string;
    attributes?: MultiInputAttributes;
  };
}

/**
 * A text input the viewer can duplicate.
 *
 * Growing the list is purely client-side. The obvious server-driven version --
 * an "add another" button that posts an event and re-renders -- would have to
 * carry every typed value back out and echo it into the new markup, or lose
 * the user's text every time they added a box. Keeping the count in local
 * state avoids both.
 *
 * Nothing else needs to know this happened: getEntirePage collects
 * `input[id]` generically at submit time, so boxes added here are picked up
 * with no protocol change. Ids are <id>, <id>2, <id>3, ... which is what the
 * backend parsers prefix-match on.
 *
 * Inputs are UNCONTROLLED, matching InputComponent -- values are read from the
 * DOM at submit, so controlling them would add re-render churn for nothing.
 * That is also why removal splices the ID LIST rather than an array of values:
 * see the comment on `remove` below.
 */
const MultiInputComponent: React.FC<MultiInputComponentProps> = ({ component, className, style }) => {
  const baseId = component.id || "multiInput";
  const placeholder = component.attributes?.placeholder ?? "";
  const extraPlaceholder = component.attributes?.extraPlaceholder ?? placeholder;
  const addLabel = component.attributes?.addLabel ?? "+ Add another";
  const maxInputs = Number(component.attributes?.maxInputs ?? 10) || 10;

  // The ids currently rendered, in order. The first is always the base id,
  // because the backend treats it as the primary field.
  const [ids, setIds] = useState<string[]>([baseId]);

  // The next suffix is derived from the ids that actually exist, computed
  // INSIDE the updater.
  //
  // A separate `nextSuffix` state does not work: React batches updates, so two
  // clicks in the same batch both read the same stale value and mint the SAME
  // id. Two inputs then share an id, and removing either filters out both --
  // which is exactly what happened before this was written this way.
  //
  // Scanning for the first free suffix can reuse a number after a removal, but
  // only once it is genuinely unused, so no two live boxes can collide.
  const add = () => {
    setIds((prev) => {
      if (prev.length >= maxInputs) return prev;
      const taken = new Set(prev);
      let suffix = 2;
      while (taken.has(`${baseId}${suffix}`)) suffix++;
      return [...prev, `${baseId}${suffix}`];
    });
  };

  // Removing by id, not by index, and letting React key on the id: because the
  // inputs are uncontrolled, their text lives in the DOM node. Re-keying a
  // surviving box would remount it and silently wipe what the user typed.
  const remove = (id: string) => {
    setIds((prev) => prev.filter((existing) => existing !== id));
  };

  return (
    <div className="flex flex-col gap-2">
      {ids.map((id, index) => (
        <div key={id} className="flex items-center gap-2">
          <input
            id={id}
            name={component.name}
            type="text"
            placeholder={index === 0 ? placeholder : extraPlaceholder}
            className={className}
            style={{ ...style, flex: 1 }}
          />
          {index > 0 && (
            <button
              type="button"
              onClick={() => remove(id)}
              aria-label="Remove this claim"
              style={{
                background: "transparent",
                border: "none",
                color: "var(--text-muted)",
                cursor: "pointer",
                fontSize: "1.1rem",
                lineHeight: 1,
                padding: "0 0.25rem",
              }}
            >
              ×
            </button>
          )}
        </div>
      ))}

      {ids.length < maxInputs && (
        <button
          type="button"
          onClick={add}
          style={{
            alignSelf: "flex-start",
            background: "transparent",
            border: "1px dashed var(--text-muted)",
            borderRadius: "0.25rem",
            color: "var(--text-secondary)",
            cursor: "pointer",
            fontSize: "0.8rem",
            padding: "0.35rem 0.6rem",
          }}
        >
          {addLabel}
        </button>
      )}
    </div>
  );
};

export default MultiInputComponent;
