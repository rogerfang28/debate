import React, { useState } from "react";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";

interface ClaimParserAttributes {
  [key: string]: any;
}

interface ClaimParserComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    name?: string;
    attributes?: ClaimParserAttributes;
  };
}

const ClaimParserComponent: React.FC<ClaimParserComponentProps> = ({
  component,
  className,
  style,
}) => {
  const [value, setValue] = useState("");

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!value.trim()) return;

    console.log(`🔍 ClaimParser submitted:`, value);

    handleEvent(
      e as any,
      component,
      "onSubmit",
      component.id || "claim-parser"
    );

    // Reset after submission
    setValue("");
  };

  const placeholder =
    component.attributes?.placeholder ||
    component.placeholder ||
    "Enter article URL or paste text to parse claims...";

  return (
    <div
      id={component.id}
      className={className}
      style={style}
    >
      <div className="mb-2">
        <span className="text-sm font-semibold text-blue-300">
          Claim Parser
        </span>
        <p className="text-xs text-gray-400 mt-0.5">
          Enter an article URL or paste text to auto-extract claims and evidence
        </p>
      </div>

      <form onSubmit={handleSubmit} className="flex flex-col gap-2">
        <textarea
          value={value}
          onChange={(e) => setValue(e.target.value)}
          placeholder={placeholder}
          className="w-full bg-gray-900 border border-gray-600 rounded-md p-3 text-white text-sm placeholder-gray-500 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent resize-y min-h-[80px]"
          rows={4}
          {...component.attributes}
        />

        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            <button
              type="submit"
              className="bg-blue-600 hover:bg-blue-500 text-white text-sm font-medium px-4 py-2 rounded-md transition-colors"
            >
              Parse Claims
            </button>
            <span className="text-xs text-gray-500">
              {value.length > 0 ? `${value.length} chars` : "Empty"}
            </span>
          </div>
        </div>
      </form>
    </div>
  );
};

export default ClaimParserComponent;
