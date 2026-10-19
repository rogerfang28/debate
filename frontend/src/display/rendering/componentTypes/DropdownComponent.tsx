import React from "react";
import { BaseComponentProps } from "./TextComponent";
import { layoutClasses } from "../buildClassName";

interface DropdownAttributes {
  options?: string;
  value?: string;
  [key: string]: any;
}

interface DropdownComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    name?: string;
    attributes?: DropdownAttributes;
  };
}

/**
 * Renders a <select>.
 *
 * The element is UNCONTROLLED, matching InputComponent. Page state is collected
 * by getEntirePage scanning the DOM for `select[id]` at submit time, so a
 * controlled value would add re-render churn without changing what gets sent.
 *
 * `options` arrives as a comma-separated attribute rather than a repeated proto
 * field -- see ComponentType.DROPDOWN in layout.proto for why.
 */
const DropdownComponent: React.FC<DropdownComponentProps> = ({ component, className, style }) => {
  const raw = component.attributes?.options ?? "";
  const options = raw
    .split(",")
    .map((option) => option.trim())
    .filter((option) => option.length > 0);

  const selected = component.attributes?.value;

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
      <select
        id={component.id}
        name={component.name}
        defaultValue={selected}
        className={layoutClasses(className)}
        style={style}
      >
        {options.map((option) => (
          <option key={option} value={option}>
            {option}
          </option>
        ))}
      </select>
    </div>
  );
};

export default DropdownComponent;
