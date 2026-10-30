import React from "react";
import { ComponentType } from "../../../../../src/gen/ts/layout_pb.ts";
import handleEvent from "../../events/handleEvent";

export interface ComponentProps {
  type: ComponentType;
  text?: string;
  value?: string | number | boolean;
  placeholder?: string;
  src?: string;
  alt?: string;
  href?: string;
  items?: any[];
  children?: ComponentProps[];
  [key: string]: any;
}

export interface BaseComponentProps {
  component: ComponentProps;
  className?: string;
  style?: React.CSSProperties;
}

const has = (className: string, pattern: string) =>
  new RegExp(`(^|\\s)(?:${pattern})(\\s|$)`).test(className);

// The theme's type scale (theme.html), chosen by the server's size class.
// The uppercase label pattern (text-xs text-gray-500 font-semibold uppercase)
// is the caption. Nothing is promoted to a heading for being short: a size
// class says how big a text is, not its length.
function typeScale(className: string): { scale: string; heading: boolean } {
  if (has(className, "uppercase")) return { scale: "caption", heading: false };
  if (has(className, "text-(?:3xl|4xl|5xl|6xl)")) return { scale: "t-h1", heading: true };
  if (has(className, "text-(?:2xl|xl)")) return { scale: "t-h2", heading: true };
  if (has(className, "text-lg")) return { scale: "t-h3", heading: false };
  if (has(className, "text-base")) return { scale: "t-body", heading: false };
  if (has(className, "text-xs")) return { scale: "t-small", heading: false };
  return { scale: "t-body-2", heading: false };
}

// A colour class (text-gray-400, text-white, text-red-400 ...) is painted by
// index.css; only a text without one gets a colour from here.
const COLOR_CLASS = "text-(?:white|black|transparent|[a-z]+-\\d+(?:/\\d+)?)";

const TextComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const text = component.text || "";
  const cls = className || "";
  const { scale, heading } = typeScale(cls);
  const strong = heading || has(cls, "font-(?:bold|semibold)");

  let color: string | undefined = style?.color as string | undefined;
  if (color === undefined && !has(cls, COLOR_CLASS) && scale !== "caption") {
    color = strong ? "var(--text)" : "var(--text-2)";
  }

  // A text the server marks with data-href is a plain link rather than an
  // event: a page of the site (the privacy page), an email address, or a
  // claim's source. A link to another website opens in a new tab, so the
  // reader keeps their place in the debate.
  const href: string | undefined = ((component as any).attributes || {})["data-href"];
  if (href) {
    const external = /^https?:\/\//i.test(href);
    return (
      <a
        id={component.id}
        href={href}
        {...(external ? { target: "_blank", rel: "noopener noreferrer" } : {})}
        onClick={(e) => e.stopPropagation()}
        className={cls ? `${scale} ${cls}` : scale}
        style={{ ...style, ...(color !== undefined ? { color } : {}) }}
      >
        {text}
      </a>
    );
  }

  const Tag = heading ? "h2" : "p";
  return (
    <Tag
      id={component.id}
      className={cls ? `${scale} ${cls}` : scale}
      style={{ ...style, ...(color !== undefined ? { color } : {}) }}
    >
      {withClaimLinks(text, ((component as any).attributes || {})["data-claim-links"])}
    </Tag>
  );
};

// The AI chat cites claims as [#12]. The server lists, in data-claim-links,
// the numbers that are claims of this debate; each of those becomes a link
// that opens the claim in the debate view. Any other bracket stays text.
function withClaimLinks(text: string, links: string | undefined): React.ReactNode {
  if (!links) return text;
  const known = new Set(links.split(","));
  const parts: React.ReactNode[] = [];
  let last = 0;
  for (const match of text.matchAll(/\[#(\d+)\]/g)) {
    const claimId = match[1];
    if (!known.has(claimId)) continue;
    const at = match.index ?? 0;
    if (at > last) parts.push(text.slice(last, at));
    parts.push(
      <button
        key={`${claimId}-${at}`}
        type="button"
        className="claim-ref"
        title="Open this claim in the debate"
        onClick={(e) => {
          e.stopPropagation();
          handleEvent(e as any, { id: `chatClaimLink_${claimId}` }, "onClick", `chatClaimLink_${claimId}`);
        }}
      >
        #{claimId}
      </button>
    );
    last = at + match[0].length;
  }
  if (last < text.length) parts.push(text.slice(last));
  return parts;
}

export default TextComponent;
