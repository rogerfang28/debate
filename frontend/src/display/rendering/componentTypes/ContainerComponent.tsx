import React, { useState } from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";

// Two opt-in behaviours, chosen by the server through attributes:
//
// data-click="true": the whole container is one press, sent as an onClick
// from the container's own id (a card on the home shelf opens its debate).
// A press on a button or field inside it is that control's own, not the
// container's.
//
// data-fold="closed" | "open": the children sit under a show/hide header
// titled data-fold-title. Opening and closing is the reader's business, so it
// never goes to the server; the page keeps it while it is rebuilt.
const ContainerComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const isFlex = className?.includes('flex');
  const isGrid = className?.includes('grid');
  // Support both "children" (from protobuf) and "components" (from pbtxt) for nesting
  const nested = component.children || component.components || [];
  const attributes = (component as any).attributes || {};
  const clickable = attributes["data-click"] === "true";
  const fold = attributes["data-fold"];

  const [open, setOpen] = useState<boolean>(fold === "open");
  const [hover, setHover] = useState<boolean>(false);

  const press = (e: React.MouseEvent | React.KeyboardEvent) => {
    const target = e.target as HTMLElement;
    if (target.closest("button, input, textarea, select, a")) return;
    handleEvent(e as any, component, "onClick", component.id || "unknown-container");
  };

  if (fold === "open" || fold === "closed") {
    return (
      <div id={component.id} className={className} style={{ ...style, overflow: "visible" }}>
        <button
          type="button"
          onClick={() => setOpen((was) => !was)}
          aria-expanded={open}
          style={{
            display: "flex",
            alignItems: "center",
            gap: "0.5rem",
            background: "transparent",
            border: "none",
            borderTop: "1px solid var(--line)",
            borderRadius: 0,
            justifyContent: "flex-start",
            width: "100%",
            padding: "0.75rem 0 0.25rem",
            color: "var(--text-2)",
            fontSize: "12px",
            fontWeight: 600,
            cursor: "pointer",
            textAlign: "left",
          }}
        >
          <span aria-hidden="true">{open ? "▼" : "▶"}</span>
          <span>{attributes["data-fold-title"] || "More"}</span>
        </button>
        {open &&
          nested.map((child: any, idx: number) => (
            <ComponentRenderer key={child.id || idx} component={child} />
          ))}
      </div>
    );
  }

  return (
    <div
      id={component.id}
      className={className}
      {...(clickable
        ? {
            role: "button",
            tabIndex: 0,
            onClick: press,
            onKeyDown: (e: React.KeyboardEvent) => {
              if (e.key === "Enter" || e.key === " ") {
                e.preventDefault();
                press(e);
              }
            },
            onMouseEnter: () => setHover(true),
            onMouseLeave: () => setHover(false),
          }
        : {})}
      style={{
        ...(!isFlex && !isGrid ? {
          display: 'flex',
          flexDirection: 'column' as const,
          gap: '0.75rem',
        } : {}),
        ...style,
        overflow: style?.overflow || 'hidden',
        ...(clickable
          ? {
              cursor: "pointer",
              transition: "box-shadow 120ms",
              // A ring rather than a border colour: the server sets the
              // border as a shorthand, which a longhand would fight.
              boxShadow: hover ? "0 0 0 1px var(--accent)" : "none",
            }
          : {}),
      }}
    >
      {nested.map((child: any, idx: number) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </div>
  );
};

export default ContainerComponent;
